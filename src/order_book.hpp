#pragma once
#include "order.hpp"
#include <map>
#include <deque>
#include <vector>
#include <unordered_map>
#include <iostream>

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint32_t quantity;
};

class OrderBook {
public:
    std::map<double, std::deque<Order>, std::greater<double>> bids;
    std::map<double, std::deque<Order>> asks;

    std::vector<Trade> add_order(Order order) {
        if (order.type == OrderType::MARKET) return match_market(order);
        return match_limit(order);
    }

    void print_book() const {
        std::cout << "=== ASK ===\n";
        for (auto &[price, orders] : asks)
            std::cout << " $" << price << " x" << total_qty(orders) << "\n";
        std::cout << "=== BID ===\n";
        for (auto &[price, orders] : bids)
            std::cout << " $" << price << " x" << total_qty(orders) << "\n";
    }

    bool cancel_order(uint64_t order_id) {
        auto it = order_index.find(order_id);
        if (it == order_index.end()) return false;

        auto [side, price] = it->second;
        if (side == Side::BUY) cancel_from(bids, price, order_id);
        else cancel_from(asks, price, order_id);

        order_index.erase(it);
        return true;
    }

private:
    std::unordered_map<uint64_t, std::pair<Side, double>> order_index;
    uint32_t total_qty(const std::deque<Order> &orders) const {
        uint32_t total = 0;
        for (auto &o : orders) total += o.quantity;
        return total;
    }

    template <typename MapType>
    std::vector<Trade> match_against(Order &order, MapType &opposite, bool is_buy)
    {
        std::vector<Trade> trades;

        while (order.quantity > 0 && !opposite.empty())
        {
            auto it = opposite.begin();
            double best_price = it->first;

            // conditions to check price crossing
            if (is_buy && order.price < best_price) break;
            if (!is_buy && order.price > best_price) break;

            auto &queue = it->second;
            while (order.quantity > 0 && !queue.empty()) {
                Order &resting = queue.front();
                uint32_t fill_qty = std::min(order.quantity, resting.quantity);

                trades.push_back({is_buy ? order.id : resting.id,
                                  is_buy ? resting.id : order.id,
                                  best_price,
                                  fill_qty});

                order.quantity -= fill_qty;
                resting.quantity -= fill_qty;

                if (resting.quantity == 0) {
                    order_index.erase(resting.id);
                    queue.pop_front();
                }
            }

            if (queue.empty()) opposite.erase(it);
        }

        return trades;
    }

    std::vector<Trade> match_limit(Order &order) {
        std::vector<Trade> trades;

        if (order.side == Side::BUY) {
            trades = match_against(order, asks, true);
        } else {
            trades = match_against(order, bids, false);
        }

        // resting unfilled quantity on book
        if (order.quantity > 0) {
            if (order.side == Side::BUY) {
                bids[order.price].push_back(order);
            } else {
                asks[order.price].push_back(order);
            }
            order_index[order.id] = {order.side, order.price};
        }

        return trades;
    }

    std::vector<Trade> match_market(Order &order) {
        order.price = (order.side == Side::BUY) ? 1e18 : 0.0;
        order.type = OrderType::LIMIT;
        return match_limit(order);
    }

    template <typename MapType>
    void cancel_from(MapType& book_side, double price, uint64_t order_id) {
        auto level_it = book_side.find(price);
        if (level_it == book_side.end()) return;

        auto& queue = level_it->second;
        for(auto q_it = queue.begin(); q_it != queue.end(); ++q_it) {
            if (q_it->id == order_id) {
                queue.erase(q_it);
                if (queue.empty()) book_side.erase(level_it);
                return;
            }
        }
    }
};
