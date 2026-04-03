#include "order_book.hpp"
#include <iostream>

int main() {
    OrderBook book;
    uint64_t id = 1;

    //resting asks
    book.add_order({id++, Side::SELL, OrderType::LIMIT, 101.0, 50,  1});
    book.add_order({id++, Side::SELL, OrderType::LIMIT, 102.0, 100, 2});

    //resting asks
    book.add_order({id++, Side::BUY,  OrderType::LIMIT, 99.0,  75,  3});
    book.add_order({id++, Side::BUY,  OrderType::LIMIT, 98.0,  200, 4});

    std::cout << "Book before match:\n";
    book.print_book();

    //buy crossing spread
    auto trades = book.add_order({id++, Side::BUY, OrderType::LIMIT, 101.0, 30, 5});

    std::cout << "\nTrades executed:\n";
    for (auto& t : trades)
        std::cout << "  buy#" << t.buy_order_id
                  << " x sell#" << t.sell_order_id
                  << " @ $" << t.price
                  << " qty=" << t.quantity << "\n";

    std::cout << "\nBook after match:\n";
    book.print_book();
}