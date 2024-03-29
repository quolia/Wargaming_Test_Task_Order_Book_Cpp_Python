#pragma once
#ifndef _WTEST_ACCUM_H
#define _WTEST_ACCUM_H

#ifdef MT
#include <mutex>
#endif
#include "OrderBook.h"

namespace WG_ORDERBOOK
{
	using namespace std;

	/// <summary> Orders time and price accumulator. </summary>
	class accumulator
	{
		shared_ptr<order_book_iface> _order_book;	/// <summary> Pointer to underlying order book container. </summary>
		double _accumulator_price;					/// <summary> Accumulated value of prices. </summary>
		unsigned long long _accumulator_time;		/// <summary> Accumulated value of time. </summary>
		timestamp_type _last_timestamp;				/// <summary> Last time maximum price was changed. </summary>

#ifdef MT
		mutex _lock; /// <summary> Operations mutex. </summary>
#endif

	public:

		/// <summary> Default ctor. </summary>
		accumulator() noexcept
		{
			reset();
		}

		/// <summary> Init accumulator. </summary>
		/// <param name="order_book"> Order book interface. </param>
		void init(shared_ptr<order_book_iface>& order_book) noexcept
		{
			_order_book = order_book;
		}

		/// <summary> Reset accumulator state. </summary>
		void reset() noexcept
		{
#ifdef MT
			lock_guard<mutex> lock(_lock);
#endif
			_order_book.reset();

			_accumulator_price = 0;
			_accumulator_time = 0;
			_last_timestamp = INVALID_TIMESTAMP;
		}

		/// <summary> Adds order to accumulator. </summary>
		/// <param name="order"> Order to add. </param>
		void add_order(const order_item& order)
		{
#ifdef MT
			lock_guard<mutex> lock(_lock);
#endif
			if (!_order_book)
			{
				throw exception("Accumulator not inited.");
			}

			// Check if new order is top-price order and accumulate previous top-price order time. 

			double current_max_price = _order_book->max_price_order().price();

			if (order.price() > current_max_price)
			{
				if (current_max_price > 0)
				{
					auto time = order.timestamp() - _last_timestamp;
					_accumulator_price += current_max_price * time;
					_accumulator_time += time;
				}

				// Update last timestamp of top-price order changing.
				_last_timestamp = order.timestamp();
			}

			_order_book->add(order);
		}

		/// <summary> Removes order from accumulator. </summary>
		/// <param name="id"> Previously added order id. </param>
		/// <param name="timestamp"> Current timestamp. </param>
		void remove_order(unsigned id, timestamp_type timestamp)
		{
#ifdef MT
			lock_guard<mutex> lock(_lock);
#endif
			if (!_order_book)
			{
				throw exception("Accumulator is not inited.");
			}

			// Check if the order being removed is the top-price order.
			const order_item& max_price_order = _order_book->max_price_order();
			if (max_price_order.id() == id)
			{
				if (timestamp < _last_timestamp)
				{
					throw exception("Time line inconsistent.");
				}
				else if (timestamp != _last_timestamp) // If not removed at the same time.
				{
					// Accumulate top-price order time and update time of top-price order changing.
					auto time = timestamp - _last_timestamp;
					_accumulator_price += max_price_order.price() * time;
					_accumulator_time += time;
					_last_timestamp = timestamp;
				}
			}

			// Remove the order from the book.
			_order_book->remove(id);
		}

		/// <summary> Returns time-weighted average highest price of orders. </summary>
		/// <returns> Time-weighted average highest price of orders. </returns>
		double average_highest_price() noexcept
		{
#ifdef MT
			lock_guard<mutex> lock(_lock);
#endif
			return 0 == _accumulator_time ? 0 : _accumulator_price / _accumulator_time;
		}
	};
}

#endif // _WTEST_ACCUM_H