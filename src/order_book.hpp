#pragma once
#include "order.hpp"
#include <map>
#include <deque>
#include <vector>
#include <iostream>

struct Trade
{
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint32_t quantity;
};

class OrderBook
{
public:
    std::map<double, std::deque<Order>, std::greater<double>> bids;
    std::map<double, std::deque<Order>> asks;

    std::vector<Trade> add_order(Order order)
    {
        if (order.type == OrderType::MARKET)
        {
            return match_market(order);
        }
        else
        {
            return match_limit(order);
        }
    }

    void print_book() const
    {
        std::cout << "=== ASK ===\n";
        for (auto &[price, orders] : asks)
            std::cout << " $" << price << " x" << total_qty(orders) << "\n";
        std::cout << "=== BID ===\n";
        for (auto &[price, orders] : bids)
            std::cout << " $" << price << " x" << total_qty(orders) << "\n";
    }

private:
    uint32_t total_qty(const std::deque<Order> &orders) const
    {
        uint32_t total = 0;
        for (auto &o : orders)
            total += o.quantity;
        return total;
    }

    std::vector<Trade> match_limit(Order &order)
    {
        std::vector<Trade> trades;

        if (order.side == Side::BUY)
        {
            while (order.quantity > 0 && !asks.empty())
            {
                auto it = asks.begin();
                if (order.price < it->first)
                    break;

                auto &queue = it->second;
                while (order.quantity > 0 && !queue.empty())
                {
                    Order &resting = queue.front();
                    uint32_t fill = std::min(order.quantity, resting.quantity);
                    trades.push_back({order.id, resting.id, it->first, fill});
                    order.quantity -= fill;
                    resting.quantity -= fill;
                    if (resting.quantity == 0)
                        queue.pop_front();
                }
                if (queue.empty())
                    asks.erase(it);
            }
            if (order.quantity > 0)
                bids[order.price].push_back(order);
        }
        else
        {
            while (order.quantity > 0 && !bids.empty())
            {
                auto it = bids.begin();
                if (order.price > it->first)
                    break;

                auto &queue = it->second;
                while (order.quantity > 0 && !queue.empty())
                {
                    Order &resting = queue.front();
                    uint32_t fill = std::min(order.quantity, resting.quantity);
                    trades.push_back({resting.id, order.id, it->first, fill});
                    order.quantity -= fill;
                    resting.quantity -= fill;
                    if (resting.quantity == 0)
                        queue.pop_front();
                }
                if (queue.empty())
                    bids.erase(it);
            }
            if (order.quantity > 0)
                asks[order.price].push_back(order);
        }

        return trades;
    }

    std::vector<Trade> match_market(Order &order)
    {
        // match market orders
        order.price = (order.side == Side::BUY) ? 1e18 : 0.0;
        order.type = OrderType::LIMIT;
        return match_limit(order);
    }
};
