#include <iostream>
#include <fstream>
#include <string>
#include "httplib.h"   // Сервер
#include "json.hpp"    // JSON парсер
#include "Database.h"

using namespace std;
using json = nlohmann::json;

json::array_t getArrayJson(const std::vector<Record>& records)
{
    json j_arr = json::array();

    for (const auto& record : records)
    {
        j_arr.push_back({
            {"id", record.id},
            {"title", record.title},
            {"price", record.price},
            {"quantity", record.quantity}
        });
    }

    return j_arr;
}

int main()
{
    httplib::Server svr;
    Database db;

    std::cout << "Server is starting..." << std::endl;


    svr.Get("/", [](const httplib::Request&, httplib::Response& res)
    {
        std::ifstream ifs("index.html");
        if (ifs.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            res.set_content(content, "text/html");
        }
        else
        {
            res.set_content("Error: index.html not found. Make sure it is in the same folder.", "text/plain");
        }
    });


    svr.Get("/api/all", [&](const httplib::Request&, httplib::Response& res)
    {
        // 1. Получи вектор записей: db.getAll()
        // 2. Создай json массив: json j_arr = json::array();
        // 3. В цикле пройдись по вектору и добавь объекты в массив:
        //    j_arr.push_back({ {"id", r.id}, {"title", r.title}, ... });
        // 4. Отправь ответ: res.set_content(j_arr.dump(), "application/json");
        json j_arr = getArrayJson(db.getAll());

        res.set_content(j_arr.dump(), "application/json");
    });


    svr.Post("/api/add", [&](const httplib::Request& req, httplib::Response& res)
    {
        try
        {
            auto j = json::parse(req.body);
            int32_t id = j["id"];

            if (id <= 0)
            {
                res.status = 400;
                res.set_content("Id must be > 0", "text/plain");
            }

            bool success = db.insert(j["id"], j["title"], j["price"], j["quantity"]);

            if (success == true)
            {
                res.set_content("OK", "text/plain");
            }
            else
            {
                res.status = 400;
                res.set_content("Duplicate ID", "text/plain");
            }
        } catch (const std::exception& e)
        {
            res.set_content(std::string("Error") + e.what(),  "text/plain");
        }
    });


    svr.Post("/api/search/id", [&](const httplib::Request& req, httplib::Response& res)
    {
        // 1. Распарси json, достань id.
        // 2. Создай переменную int reads = 0;
        // 3. Вызови db.findById(id, reads);
        // 4. Сформируй json ответ:
        //    json resp;
        //    resp["reads"] = reads;
        //    if (result != nullptr) {
        //        resp["found"] = true;
        //        resp["data"] = { {"title", result->title}, ... };
        //    } else {
        //        resp["found"] = false;
        //    }
        // 5. Отправь resp.dump()

        // ТВОЙ КОД ЗДЕСЬ:
        auto j = json::parse(req.body);
        int32_t reads = 0;
        Record* record_ptr = db.findById(j["id"], reads);
        json resp;
        resp["reads"] = reads;

        if (record_ptr != nullptr)
        {
            resp["found"] = true;
            resp["data"] = {
                {"id", record_ptr->id},
                {"title", record_ptr->title},
                {"price", record_ptr->price},
                {"quantity", record_ptr->quantity}
            };
        }
        else
        {
            resp["found"] = false;
        }

        res.set_content(resp.dump(), "application/json");
    });


    svr.Post("/api/search/title", [&](const httplib::Request& req, httplib::Response& res)
    {
        // 1. Достань title из json.
        // 2. Получи вектор: db.findByTitle(title);
        // 3. Упакуй вектор в json массив (как в api/all).
        // 4. Отправь.
        auto j = json::parse(req.body);

        json j_arr = getArrayJson(db.findByTitle(std::string(j["title"])));

        res.set_content(j_arr.dump(), "application/json");
    });

    svr.Post("/api/search/price", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto j = json::parse(req.body);

        json j_arr = getArrayJson(db.findByPrice(j["price"]));

        res.set_content(j_arr.dump(), "application/json");
    });

    svr.Post("/api/search/quantity", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto j = json::parse(req.body);

        json j_arr = getArrayJson(db.findByQuantity(j["quantity"]));

        res.set_content(j_arr.dump(), "application/json");
    });


    svr.Post("/api/delete/id", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto j = json::parse(req.body);
        int32_t id = j["id"];

        if (db.deleteById(id))
        {
            res.set_content("Deleted (O(1))", "text/plain");
        }
        else
        {
            res.status = 404;
            res.set_content("ID not found", "text/plain");
        }
    });

    svr.Post("/api/delete/title", [&](const auto& req, auto& res)
    {
        int32_t count = db.deleteByTitle(json::parse(req.body)["title"]);
        res.set_content(std::to_string(count), "text/plain");
    });

    svr.Post("/api/delete/quantity", [&](const auto& req, auto& res)
    {
        int32_t count = db.deleteByQuantity(json::parse(req.body)["quantity"]);
        res.set_content(std::to_string(count), "text/plain");
    });

    svr.Post("/api/delete/price", [&](const httplib::Request& req, httplib::Response& res)
    {
        int32_t count = db.deleteByPrice(json::parse(req.body)["price"]);
        res.set_content(std::to_string(count), "text/plain");
    });


    svr.Post("/api/clear", [&](const auto&, auto& res)
    {
        db.clear();
        res.set_content("Database cleared", "text/plain");
    });

    svr.Get("/api/backup", [&](const auto&, auto& res)
    {
        db.backup();
        res.set_content("Backup created", "text/plain");
    });

    svr.Get("/api/restore", [&](const auto&, auto& res)
    {
        db.restore();
        res.set_content("Restored from backup", "text/plain");
    });

    svr.Get("/api/export", [&](const auto&, auto& res)
    {
        db.exportCSV();
        res.set_content("Exported to CSV", "text/plain");
    });

    std::cout << "Server running on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}
