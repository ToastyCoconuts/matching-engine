#include "../src/order_book.hpp"
#include <cassert>
#include <iostream>

uint64_t next_id = 1;
uint64_t ts = 0;

void reset() {
    next_id = 1;
    ts = 0;
}

Order make_limit(Side side, double price, uint32_t qty) {
    return {next_id++, side, OrderType::LIMIT, price, qty, ts++};
}

Order make_market(Side side, uint32_t qty) {
    return {next_id++,side, OrderType::MARKET, 0.0, qty, ts++};
}

void test_basic_match() {
    reset();
    OrderBook book;
    Order sell = make_limit(Side::SELL, 100.0, 10);
    Order buy = make_limit(Side::BUY, 100.0, 10);
    uint64_t sell_id = sell.id;
    uint64_t buy_id = buy.id;
    book.add_order(sell);
    auto trades = book.add_order(buy);

    assert(trades.size() == 1);
    assert(trades[0].buy_order_id == buy_id);
    assert(trades[0].sell_order_id == sell_id);
    assert(trades[0].price == 100.0);
    assert(trades[0].quantity == 10);
    assert(book.bids.empty());
    assert(book.asks.empty());
    std::cout << "PASS: basic match\n";
}

void test_no_match() {
    reset();
    OrderBook book;
    Order sell = make_limit(Side::SELL, 105.0, 10);
    Order buy = make_limit(Side::BUY, 100.0, 5);
    uint64_t sell_id = sell.id;
    uint64_t buy_id = buy.id;
    book.add_order(sell);
    auto trades = book.add_order(buy);

    assert(trades.empty());
    assert(book.asks.begin()->first == 105.0);
    assert(book.asks.begin()->second.front().id == sell_id);
    assert(book.bids.begin()->first == 100.0);
    assert(book.bids.begin()->second.front().id == buy_id);
    std::cout << "PASS: no match (no price cross)\n";
}

void test_partial_fill() {
    reset();
    OrderBook book;
    Order sell = make_limit(Side::SELL, 100.0, 10);
    Order buy = make_limit(Side::BUY, 100.0, 4);
    uint64_t sell_id = sell.id;
    uint64_t buy_id = buy.id;
    book.add_order(sell);
    auto trades = book.add_order(buy);

    assert(trades.size() == 1);
    assert(trades[0].buy_order_id == buy_id);
    assert(trades[0].sell_order_id == sell_id);
    assert(trades[0].quantity == 4);
    assert(book.asks.begin()->second.front().id == sell_id);
    assert(book.asks.begin()->second.front().quantity == 6);
    assert(book.bids.empty());
    std::cout << "PASS: partial fill\n";
}

void test_price_time_priority() {
    reset();
    OrderBook book;
    Order a = make_limit(Side::SELL, 100.0, 5);
    Order b = make_limit(Side::SELL, 100.0, 5);
    uint64_t a_id = a.id;
    uint64_t b_id = b.id;
    book.add_order(a);
    book.add_order(b);
    auto trades = book.add_order(make_limit(Side::BUY, 100.0, 5));

    assert(trades.size() == 1);
    assert(trades[0].quantity == 5);
    assert(trades[0].sell_order_id == a_id);
    //A filled firstm and b should remain
    assert(book.asks.begin()->second.size() == 1);
    assert(book.asks.begin()->second.front().id == b_id);
    std::cout << "PASS: price-time priority\n";

}

void test_multi_level_fill() {
    reset();
    OrderBook book;
    Order sell_100 = make_limit(Side::SELL, 100.0, 5);
    Order sell_101 = make_limit(Side::SELL, 101.0, 5);
    Order buy = make_limit(Side::BUY, 101.0, 8);
    uint64_t sell_100_id = sell_100.id;
    uint64_t sell_101_id = sell_101.id;
    uint64_t buy_id = buy.id;
    book.add_order(sell_100);
    book.add_order(sell_101);
    auto trades = book.add_order(buy);

    assert(trades.size() == 2);
    assert(trades[0].price == 100.0);
    assert(trades[0].quantity == 5);
    assert(trades[0].buy_order_id == buy_id);
    assert(trades[0].sell_order_id == sell_100_id);
    assert(trades[1].price == 101.0);
    assert(trades[1].quantity == 3);
    assert(trades[1].buy_order_id == buy_id);
    assert(trades[1].sell_order_id == sell_101_id);
    assert(book.asks.begin()->second.front().id == sell_101_id);
    assert(book.asks.begin()->second.front().quantity == 2);
    std::cout << "PASS: multi-level fill\n";
}

void test_market_order() {
    reset();
    OrderBook book;
    Order sell = make_limit(Side::SELL, 100.0, 10);
    Order buy = make_market(Side::BUY, 10);
    uint64_t sell_id = sell.id;
    uint64_t buy_id = buy.id;
    book.add_order(sell);
    auto trades = book.add_order(buy);

    assert(trades.size() == 1);
    assert(trades[0].buy_order_id == buy_id);
    assert(trades[0].sell_order_id == sell_id);
    assert(trades[0].price == 100.0);
    assert(trades[0].quantity == 10);
    assert(book.asks.empty());
    std::cout << "PASS: market order\n";
}

void test_cancel_order() {
    reset();
    OrderBook book;
    Order sell = make_limit(Side::SELL, 100.0, 10);
    uint64_t id = sell.id;
    book.add_order(sell);

    assert(book.cancel_order(id) == true);
    assert(book.asks.empty());
    std::cout << "PASS: cancel order\n";
}

void test_cancel_nonexistent() {
    reset();
    OrderBook book;
    assert(book.cancel_order(99999) == false);
    std::cout << "PASS: cancel nonexistent order\n";
}

void test_cancel_middle_of_queue() {
    reset();
    OrderBook book;
    Order a = make_limit(Side::SELL, 100.0, 5);
    Order b = make_limit(Side::SELL, 100.0, 10);
    Order c = make_limit(Side::SELL, 100.0, 7);
    uint64_t a_id = a.id;
    uint64_t b_id = b.id;
    uint64_t c_id = c.id;
    book.add_order(a);
    book.add_order(b);
    book.add_order(c);

    book.cancel_order(b_id);
    assert(book.asks.begin()->second.size() == 2);
    assert(book.asks.begin()->second.front().id == a_id);
    assert(book.asks.begin()->second.back().id == c_id);
    std::cout << "PASS: cancel middle of queue\n";
}

int main() {
    test_basic_match();
    test_no_match();
    test_partial_fill();
    test_price_time_priority();
    test_multi_level_fill();
    test_market_order();
    test_cancel_order();
    test_cancel_nonexistent();
    test_cancel_middle_of_queue();
    std::cout << "\nAll tests passed\n";
    return 0;
}
