#! /usr/bin/env python3

""" grblcon: an enhanced GRBL communication shell

"""

import threading
from threading import Event
import os
import signal
import time

import serial
import urwid
from urwid import raw_display

# TODO: Think about replacing Positions with a named tuple


class Positions:
    """ A simple object to parse and maintain positions
    """

    # pylint: disable=too-few-public-methods

    def __init__(self, pos_string='0,0,0,0'):

        # pylint: disable=invalid-name
        self.x, self.y, self.z, self.c = [
            float(x) for x in pos_string.split(',')
        ]

    def __str__(self):
        return '\n'.join([
            'Positions',
            '---------',
            'X: {}'.format(self.x),
            'Y: {}'.format(self.y),
            'Z (Gripper): {}'.format(self.z),
            'C (Carousel): {}'.format(self.c),
            '\n'
        ])


class Status:
    """Data container object which holds grbl state status and can return
    a string appropriate for display in the status header.

    """

    # pylint: disable=too-few-public-methods

    def __init__(self):
        self.state = None
        self.mach_positions = None
        self.work_positions = None
        self.line = None
        self.limit_flags = None
        self.reset()

    def reset(self):
        """ Resets the state of the status object to sane defaults

        :returns: None
        :rtype: None

        """
        self.state = 'Disconnected'
        self.mach_positions = Positions()
        self.work_positions = Positions()
        self.line = '0'
        self.limit_flags = '0000000000'

    def __str__(self):
        return '[Status] ' + ' | '.join([
            "State: {}".format(self.state),
            "MPositions: {}, {}, {}, {}".format(
                self.mach_positions.x,
                self.mach_positions.y,
                self.mach_positions.z,
                self.mach_positions.c
            ),
            "WPositions: {}, {}, {}, {}".format(
                self.work_positions.x,
                self.work_positions.y,
                self.work_positions.z,
                self.work_positions.c
            ),
            "Line: {}".format(self.line),
            "Limit Flags: {}".format(self.limit_flags)
        ])


def _update_text(max_rows, original, addition):
    """ Joins two bodies of text and limits the total number of rows to max_rows

    :param max_rows: int - Maximum number of final rows of text
    :param original: str - original body of text
    :param addition: str - text to append to the original
    :returns: The new row-limited text
    :rtype: str

    """
    lines = original.splitlines() + addition.splitlines()
    lines = lines[-max_rows:]

    return '\n'.join(lines)


class GrblCon:
    """Main application object.

    Contains the application state, as well as an external interface
    to start the urwid main loop

    """
    # pylint: disable=too-many-instance-attributes

    _palette = [
        ('status', 'black', 'light gray'),
        ('text_entry', 'white', 'black'),
        ('text_display', 'white', 'black')
    ]

    # NOTE: the /{number} in the dictionary value represents the
    # command's arity.
    _command_help = {
        'break': '/0 Print a pagebreak',
        'clear': '/0 Clear the screen',
        'close': '/0 Close the current grbl connection',
        'help': '/0 Display this dialog',
        'home': '/0..4 (X/Y/Z/C) Homes the specified axis. Multiple axis may be provided (or none to home all)',
        'load_gcode': '/1 Send the specified gcode file to grbl',
        'log': '/0 Display the event log',
        'open': '/2 (dev, baud) Open a grbl connection to dev@baud',
        'quit': '/0 Close any active grbl connection and exit',
        'readadc': '/0 Performs a read of the on-board ADCs',
        'reset': '/1 (hard?) Reset GRBL via command. If `reset hard`, performs a hardware reset',
        'run': '/0 Begins execution of loaded gcode program',
        'toggle_quiet': '/0 Toggle squelching of status messages',
        '\\': '/? Sends a raw command strait to grbl',
    }

    def __init__(self):
        # Serial related bits
        self._port = None
        self._serial_thread = None
        self._end_evt = Event()
        self._reset_req = Event()

        # GRBL Status
        self._status = Status()

        # Application state
        self._quiet = True
        self._event_log = ''

        # Urwid stuff
        self._status_display = urwid.Text(('status', ''))

        # Expand and fill out the text box so that text will begin at
        # the bottom of the text display area
        self._text_display = urwid.Text('')

        # Set our prompt
        self._text_entry = urwid.Edit('>>> ')
        self._screen = raw_display.Screen()

        frame = urwid.Frame(
            header=self._status_display,
            body=urwid.Filler(self._text_display),
            footer=self._text_entry,
            focus_part='footer'
        )

        self.view = urwid.Pile([frame])
        self._write_fd = None

        self.loop = urwid.MainLoop(self.view, self._palette,
                                   unhandled_input=self._process_user_input)

        signal.signal(signal.SIGINT, self._sigint_handler)
        self._update_status()
        self._clear_screen()

    def _log_event(self, level, evt_str):
        log_time = time.time()

        log_header = (
            '[{time}]({level})'.format(time=log_time, level=level).ljust(30)
        )

        log_fmt = log_header + '{data}'

        log_entry = '\n'.join([
            log_fmt.format(data=x)
            for x in evt_str.splitlines()
        ])

        self._event_log = _update_text(self.rows - 2,
                                       self._event_log,
                                       log_entry)

        if level == 'CRIT':
            self._update_text_display(log_entry)

    def _log_info(self, evt_str):
        self._log_event('INFO', evt_str)

    def _log_error(self, evt_str):
        self._log_event('ERROR', evt_str)

    def _log_crit(self, evt_str):
        self._log_event('CRIT', evt_str)

    def _sigint_handler(self, sig, frame):
        # pylint: disable=unused-argument
        """Handles a ctrl-c from the user in a clean way

        """
        self.quit()

    def _delimit_display(self):
        """Prints a page breaking line

        :returns: None
        :rtype: None

        """
        self._update_text_display('-' * self.cols)

    def _close_serial(self):
        """Attempts to gracefully close the serial connection

        :returns: None
        :rtype: None

        """
        # If we're talking to a controller, try to gracefully kill the
        # serial connection and watch pipe
        if self.connected:
            self._end_evt.set()
            self._serial_thread.join()
            self._port.flush()
            self._port.close()
            self.loop.remove_watch_pipe(self._write_fd)
            self._serial_thread = None

        self._status.reset()
        self._update_status()

    @property
    def rows(self):
        """Gets the current number of rows on screen

        :returns: Number of screen rows
        :rtype: int

        """
        _, rows = self._screen.get_cols_rows()

        return rows

    @property
    def cols(self):
        """Gets the current number of columns on screen

        :returns: Number of screen columns
        :rtype: int

        """
        cols, _ = self._screen.get_cols_rows()

        return cols

    @property
    def connected(self):
        """ Returns the status of the serial connection

        :returns: connected status
        :rtype: bool

        """
        return self._serial_thread is not None

    def _write(self, data):
        """Wraps our serial port so that connection status and encoding are
        automatically handled

        :param data: str - data to write

        """

        if self.connected:
            self._port.write(bytes(data, 'ISO-8859-1'))
        else:
            self._log_crit("Not connected to GRBL!")

    def _writeline(self, data):
        """Writes provided string to the serial port (if connected) and appends a newline

        :param data: str - data to write

        """

        self._write(data + '\n')

    def _clear_screen(self):
        """ Resets the text display area

        :returns: None
        :rtype: None

        """
        self._text_display.set_text('\n' * self.rows)

    def quit(self):
        """ Cleans up and tries to quit the program

        :returns: None
        :rtype: None

        """
        self._close_serial()
        raise urwid.ExitMainLoop

    def _handle_load_gcode(self, *args):  # pylint: disable=unused-argument
        filename = args[0]

        filename = os.path.expanduser(filename)
        filename = os.path.abspath(filename)

        with open(filename) as gcfil:
            lines = gcfil.readlines()

        lnum = 0
        for line in lines:
            numbered_line = 'N{} {}'.format(lnum, line)
            lnum += 1
            self._update_text_display(numbered_line)
            self._write(numbered_line + '\n')

    def _handle_run(self, *args):  # pylint: disable=unused-argument
        self._write('~')

    def _handle_clear(self, *args):  # pylint: disable=unused-argument
        self._clear_screen()

    def _handle_break(self, *args):  # pylint: disable=unused-argument
        self._delimit_display()

    def _handle_help(self, *args):  # pylint: disable=unused-argument
        help_str = '\n'.join([
            'Available Commands:',
            '-------------------',
        ])

        esc_handlers = [x.replace('_handle_', '') for x
                        in dir(self)
                        if x.startswith('_handle')]

        esc_handlers.append('\\')

        self._update_text_display(help_str)
        max_cmd_len = max([len(x) for x in self._command_help])
        for handler in esc_handlers:
            help_str = '{} - {}'.format(handler.ljust(max_cmd_len),
                                        self._command_help[handler])
            self._update_text_display(help_str)

        help_footer = ('\nNote: Commands are listed as '
                       '{command} - /<arity> (args) [help]\n'
                       'args should be space separated!')
        self._update_text_display(help_footer)

    def _handle_home(self, *args):
        cmd = '$H'

        for arg in args:
            if arg in 'XYZCxyzc' and not arg in cmd:
                cmd += arg

        self._write(cmd)

    def _handle_quit(self, *args):  # pylint: disable=unused-argument
        self.quit()

    def _handle_toggle_quiet(self, *args):  # pylint: disable=unused-argument
        self._quiet = not self._quiet

    def _handle_close(self, *args):  # pylint: disable=unused-argument
        self._log_info('Closing GRBL Connection')
        self._close_serial()

    def _handle_readadc(self, *args):  # pylint: disable=unused-argument
        if not self.connected:
            return

        self._write('|')

    def _handle_reset(self, *args):  # pylint: disable=unused-argument
        if not self.connected:
            return

        if len(args) and args[0] == 'hard':
            self._reset_req.set()
        else:
            # Send a ctrl-x
            self._write('\x18')

    def _handle_open(self, *args):  # pylint: disable=unused-argument
        self._open_port(*args)

    def _handle_log(self, *args):  # pylint: disable=unused-argument
        self._delimit_display()
        self._update_text_display(self._event_log)

    def _update_text_display(self, text):
        """ Updates the central display widget with an additional line of text

        :param text: a single line of text to insert at the bottom
        :returns: None
        :rtype: None

        """
        new_text = _update_text(self.rows - 2,  # Account for the header/footer
                                self._text_display.text,
                                text)

        self._text_display.set_text(new_text)

    def _process_escape_seq(self, text):
        # Strip off the escape character
        text = text[1:]

        self._update_text_display(text)
        self._writeline(text)

    def _process_cmd(self, text):
        try:
            cmd, *cmd_args = text.split()
        except ValueError:
            # If there are no values to unpack (empty string), just return
            return

        cmd_handler = '_handle_{}'.format(cmd)
        if hasattr(self, cmd_handler):
            getattr(self, cmd_handler)(*cmd_args)

    def _process_user_input(self, key):
        """Sorts and handles user input lines

        :param key: string containing the latest keyboard input
        :returns: None
        :rtype: None

        """
        if key == 'enter':
            user_text = self._text_entry.get_edit_text()
            self._text_entry.set_edit_text('')

            if user_text.startswith('\\'):
                self._process_escape_seq(user_text)
            else:
                self._process_cmd(user_text)

    def _update_status(self):
        statstring = str(self._status).ljust(self.cols)
        self._status_display.set_text(('status', statstring))

    def _process_realtime_state(self, line):
        """Handles realtime state updates

        :param line: a string containing a state message
        :returns: None
        :rtype: None

        """
        # rip out the tag delmiters
        line = line.strip('<>')

        # Split the line into discrete sections
        (self._status.state,
         m_pos,
         w_pos,
         self._status.line) = line.split(':')

        self._status.mach_positions = Positions(m_pos)
        self._status.work_positions = Positions(w_pos)

    def _process_limit_flags(self, line):
        """Handles incoming limit flag information

        :param line: a string containing the limit flag message
        :returns: None
        :rtype: None

        """
        self._status.limit_flags = line.strip('/')

    def _check_and_clean(self, line):
        """Performs a checksum on serial line data and filters bad responses

        :param line: A string in the format [data0..n][checksum][\n]
        :returns: None if bad checksum, else stripped string
        :rtype: None/str

        """
        *line, checksum = line

        calcsum = sum(line) & 0xff
        residue = calcsum ^ checksum

        if residue:
            err = 'Bad Checksum. {} != {}. Data: {}'.format(
                calcsum,
                checksum,
                ''.join([chr(x) for x in line]).strip()
            )
            self._log_error(err)
            return None

        return line

    def _process_serial_line(self, bytestring):
        """Sorts and handles lines of data from the grbl firmware

        :param bytes: a bytearray in the form of a newline terminated string
        :returns: None
        :rtype: None

        """
        for line in bytestring.decode('ISO-8859-1').splitlines():

            # Short circuit in case we got a bad checksum
            if not line:
                return

            # Assume that we'll filter the message. This will be used
            # later to see if we should update the display with the data
            # that came back
            filtered = True

            if line.startswith('<'):   # Real time state
                self._process_realtime_state(line)
            elif line.startswith('/'):  # Limit pin state
                self._process_limit_flags(line)
            else:
                filtered = False
                self._update_text_display(line)

            if filtered and not self._quiet:
                self._update_text_display(line)

            self._update_status()

    def _serial_rx(self):
        if not self._port:
            return

        self._write_fd = self.loop.watch_pipe(self._process_serial_line)
        while not self._end_evt.isSet():
            # Read a line of data + its checksum
            line = self._port.readline()
            line += self._port.read()

            line = self._check_and_clean(line)

            if not line:
                continue

            if self._write_fd and line:
                os.write(self._write_fd, bytes(line))

            if self._reset_req.is_set():
                self._log_info("Resetting")
                self._port.setDTR(False)
                self._port.setDTR(True)
                self._log_info("Reset Complete")
                self._reset_req.clear()

    def _open_port(self, portstr='/dev/ttyUSB0', baud=38400):
        """ Opens a serial port and stores a handle to it

        :param portstr: str - The port to open (e.g. /dev/ttyACM0
        :param baud: int - Baud Rate [9600, 38400, 115200, etc]
        :returns: None
        :rtype: None

        """
        if self.connected:
            self._log_crit('grbl already connected!')
        else:
            self._log_info(
                'Opening GRBL connection @ {}::{}'.format(portstr, baud)
            )
            try:
                self._port = serial.Serial(baudrate=baud, dsrdtr=True)
                self._port.port = portstr
                self._port.open()
                self._serial_thread = threading.Thread(target=self._serial_rx)
                self._end_evt.clear()
                self._serial_thread.start()
            except (serial.serialutil.SerialException, FileNotFoundError):
                self._log_crit("Unable to open port: {}".format(portstr))

    def run(self):
        """Starts the main urwid loop and does not return

        :returns: None
        :rtype: None

        """
        self.loop.run()

if __name__ == '__main__':
    GrblCon().run()
