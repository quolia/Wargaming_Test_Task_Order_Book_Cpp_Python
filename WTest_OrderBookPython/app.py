from cli import CommandLine
from file import OrdersFile
from orderbook import OrderBookInterface, OrderBookFactory
from accumulator import Accumulator
from items import *


class Application:
    """Application class."""

    def __init__(self):
        self._is_inited = False                     # The flag is true if the application instance has been inited successfully.
        self._src_file = OrdersFile()               # File to read.
        self._order_book: OrderBookInterface = None # Pointer to order book interface.
        self._accumulator = Accumulator()           # Orders time-weight accumulator.

    def init(self, args):
        """
        Initiates application instance.
        :param args: Command line arguments.
        """
        self._is_inited = False

        cli = CommandLine()
        cli.init(args)

        print(f'Source file: {cli.src_file_name()}')
        self._src_file.open_for_read(cli.src_file_name())

        self._order_book = OrderBookFactory.get()
        self._accumulator.init(self._order_book)

        self._is_inited = True

    def run(self):
        """Reads source file till the end."""
        if not self._is_inited:
            raise Exception('Application has not been inited yet.')

        record = OrderRecord()
        while self._src_file.read_order_record(record):
            if record.is_insert():
                self._accumulator.add_order(OrderItem.from_record(record))
            else:
                self._accumulator.remove_order(record.id(), record.timestamp())

    def average_highest_price(self):
        """Returns time-weighted average highest price of orders."""
        if not self._is_inited:
            raise Exception('Application has not been inited yet.')
        return self._accumulator.average_highest_price()
