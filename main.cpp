#include <iostream>
#include <fstream>
#include <string>
#include "httplib.h"   // Сервер
#include "json.hpp"    // JSON парсер
#include "Database.h"  // Твоя база данных

using namespace std;
using json = nlohmann::json;

int main()
{
    // 1. Инициализация
    httplib::Server svr;
    Database db; // При запуске она сама найдет файл или создаст новый

    std::cout << "Server is starting..." << std::endl;

    // --- МАРШРУТ 1: Отдача интерфейса (HTML) ---
    // Это я написал за тебя. Сервер просто читает файл index.html и отдает браузеру.
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream ifs("index.html");
        if (ifs.is_open()) {
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            res.set_content(content, "text/html");
        } else {
            res.set_content("Error: index.html not found. Make sure it is in the same folder.", "text/plain");
        }
    });

    // --- МАРШРУТ 2: Получить ВСЕ записи (для отрисовки таблицы) ---
    // URL: /api/all
    svr.Get("/api/all", [&](const httplib::Request&, httplib::Response& res) {
        // 1. Получи вектор записей: db.getAll()
        // 2. Создай json массив: json j_arr = json::array();
        // 3. В цикле пройдись по вектору и добавь объекты в массив:
        //    j_arr.push_back({ {"id", r.id}, {"title", r.title}, ... });
        // 4. Отправь ответ: res.set_content(j_arr.dump(), "application/json");

        // ТВОЙ КОД ЗДЕСЬ:
    });

    // --- МАРШРУТ 3: Добавить запись ---
    // URL: /api/add (POST)
    // JSON приходит в req.body
    svr.Post("/api/add", [&](const httplib::Request& req, httplib::Response& res) {
        // 1. Распарси: auto j = json::parse(req.body);
        // 2. Достань поля: id, title, price, quantity
        // 3. Вызови: bool success = db.insert(...);
        // 4. Если success == true -> отправь "OK"
        // 5. Если false -> res.status = 400; res.set_content("Duplicate ID", "text/plain");

        // ТВОЙ КОД ЗДЕСЬ:
    });

    // --- МАРШРУТ 4: Поиск по ID (Быстрый O(1)) ---
    // URL: /api/search/id (POST)
    svr.Post("/api/search/id", [&](const httplib::Request& req, httplib::Response& res) {
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
    });

    // --- МАРШРУТ 5: Поиск по Названию (Линейный O(N)) ---
    // URL: /api/search/title (POST)
    svr.Post("/api/search/title", [&](const httplib::Request& req, httplib::Response& res) {
        // 1. Достань title из json.
        // 2. Получи вектор: db.findByTitle(title);
        // 3. Упакуй вектор в json массив (как в api/all).
        // 4. Отправь.

        // ТВОЙ КОД ЗДЕСЬ:
    });

    // --- МАРШРУТ 6: Удаление по Цене ---
    // URL: /api/delete/price (POST)
    svr.Post("/api/delete/price", [&](const httplib::Request& req, httplib::Response& res) {
        // 1. Достань price (double).
        // 2. Вызови db.deleteByPrice(price).
        // 3. Верни количество удаленных записей (преврати int в string: std::to_string(count)).

        // ТВОЙ КОД ЗДЕСЬ:
    });

    // --- СЛУЖЕБНЫЕ МАРШРУТЫ (Уже готовы, просто раскомментируй или оставь как есть) ---

    svr.Post("/api/clear", [&](const auto&, auto& res) {
        db.clear();
        res.set_content("Database cleared", "text/plain");
    });

    svr.Get("/api/backup", [&](const auto&, auto& res) {
        db.backup();
        res.set_content("Backup created", "text/plain");
    });

    svr.Get("/api/restore", [&](const auto&, auto& res) {
        db.restore();
        res.set_content("Restored from backup", "text/plain");
    });

    svr.Get("/api/export", [&](const auto&, auto& res) {
        db.exportCSV();
        res.set_content("Exported to CSV", "text/plain");
    });

    // Запуск
    std::cout << "Server running on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}