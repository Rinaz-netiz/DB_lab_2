#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <filesystem>

using namespace std;

const string kDbFile = "store.db";
const string kBackupFile = "store_backup.db";
const string kCSVFile = "data.csv";

struct Record
{
    int32_t id;
    char title[64];
    double price;
    int32_t quantity;
    bool is_deleted;
};

struct Header
{
    int32_t capacity;
    int32_t count;
};

constexpr int32_t kRecordSize = sizeof(Record);
constexpr int32_t kHeaderSize = sizeof(Header);

enum class Fields { BY_TITLE, BY_PRICE, BY_QUANTITY };

class Database
{
private:
    int32_t capacity_;
    int32_t count_;

    int32_t hash(const int32_t id) const
    {
        if (capacity_ == 0)
        {
            throw std::runtime_error("DB doesn't exist");
        }
        return std::abs(id) % capacity_;
    }

    void writeHeader() const
    {
        std::fstream file(kDbFile, std::ios::binary | std::ios::in | std::ios::out);

        if (!file.is_open())
        {
            throw std::runtime_error("File for db didn't open");
        }

        Header header{capacity_, count_};

        file.seekp(0, std::ios::beg);
        file.write(reinterpret_cast<char*>(&header), kHeaderSize);

        file.close();
    }

    void createNew(const int32_t new_capacity)
    {
        std::ofstream out(kDbFile, std::ios::binary);

        if (!out.is_open())
        {
            throw std::runtime_error("File for db didn't open");
        }

        Header header{new_capacity, 0};
        out.write(reinterpret_cast<char*>(&header), kHeaderSize);

        capacity_ = new_capacity;
        count_ = 0;

        Record record;
        record.id = 0;
        record.is_deleted = true;

        for (int32_t count = 0; count < new_capacity; ++count)
        {
            out.write(reinterpret_cast<char*>(&record), kRecordSize);
        }

        out.close();
    }

    void resize()
    {
        cout << "Resizing..." << endl;
        std::vector<Record> records = getAll();

        createNew(2 * capacity_);

        for (const auto& record : records)
        {
            insert(record.id, record.title, record.price, record.quantity);
        }
    }

    template <class T>
    vector<Record> findBy(const T& field, const Fields field_type) const
    {
        if (capacity_ == 0)
        {
            throw std::runtime_error("DB doesn't exist");
        }
        vector<Record> result;

        std::ifstream in(kDbFile, std::ios::binary);

        if (!in.is_open())
        {
            throw std::runtime_error("File for db didn't open to find by field");
        }

        in.seekg(kHeaderSize, std::ios::beg);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            in.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (record.is_deleted == true)
            {
                continue;
            }

            if constexpr (std::is_same_v<T, double>)
            {
                if (field_type == Fields::BY_PRICE && fabs(record.price - field) < 1e-9)
                    result.push_back(record);
            }
            else if constexpr (std::is_same_v<T, int32_t>)
            {
                if (field_type == Fields::BY_QUANTITY && record.quantity == field)
                    result.push_back(record);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (field_type == Fields::BY_TITLE && strcmp(record.title, field.c_str()) == 0)
                    result.push_back(record);
            }
            else
            {
                static_assert(sizeof(T) == 0, "Unsupported type passed to findBy()");
            }
        }

        return result;
    }

    template <class T>
    int32_t deleteBy(const T& field, const Fields field_type)
    {
        std::fstream file(kDbFile, std::ios::binary | std::ios::in | std::ios::out);

        if (!file.is_open())
        {
            throw std::runtime_error("File for db didn't open to delete by field");
        }

        int32_t count = 0;

        file.seekg(kHeaderSize, std::ios::beg);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            file.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (record.is_deleted == true)
            {
                continue;
            }

            bool flag = false;
            if constexpr (std::is_same_v<T, double>)
            {
                if (field_type == Fields::BY_PRICE && fabs(record.price - field) < 1e-9)
                    flag = true;
            }
            else if constexpr (std::is_same_v<T, int32_t>)
            {
                if (field_type == Fields::BY_QUANTITY && record.quantity == field)
                    flag = true;
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (field_type == Fields::BY_TITLE && strcmp(record.title, field.c_str()) == 0)
                    flag = true;
            }
            else
            {
                static_assert(sizeof(T) == 0, "Unsupported type passed to deleteBy()");
            }

            if (flag == true)
            {
                record.is_deleted = true;
                file.seekp(-kRecordSize, std::ios::cur);
                file.write(reinterpret_cast<char*>(&record), kRecordSize);

                --count_;
                ++count;
            }
        }

        writeHeader();

        return count;
    }

public:
    Database()
    {
        std::ifstream in(kDbFile, std::ios::binary);

        if (in.is_open())
        {
            Header header;

            in.read(reinterpret_cast<char*>(&header), kHeaderSize);
            capacity_ = header.capacity;
            count_ = header.count;
        }
        else
        {
            createNew(100);
        }
    }

    bool create()
    {
        if (std::filesystem::exists(kDbFile))
        {
            return false;
        }
        createNew(100);
        return true;
    }

    void drop()
    {
        if (std::filesystem::exists(kDbFile))
        {
            std::filesystem::remove(kDbFile);
        }
        capacity_ = 0;
        count_ = 0;
    }

    bool insert(int32_t id, string title, double price, int32_t quantity)
    {
        if (id <= 0)
        {
            return false;
        }

        const int32_t ind = hash(id);

        if (count_ > capacity_ * 0.7)
        {
            resize();
        }

        std::fstream file(kDbFile, std::ios::binary | std::ios::in | std::ios::out);

        if (!file.is_open())
        {
            throw std::runtime_error("File for db didn't open to insert");
        }

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            file.seekg(kHeaderSize + ((ind + idx) % capacity_) * kRecordSize, std::ios::beg);
            file.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (!record.is_deleted && record.id == id)
            {
                return false;
            }

            if (record.is_deleted)
            {
                record.is_deleted = false;
                record.id = id;

                std::strncpy(record.title, title.c_str(), sizeof(record.title));
                record.title[sizeof(record.title) - 1] = '\0';

                record.price = price;
                record.quantity = quantity;

                file.seekp(kHeaderSize + ((ind + idx) % capacity_) * kRecordSize, std::ios::beg);
                file.write(reinterpret_cast<char*>(&record), kRecordSize);

                ++count_;
                writeHeader();

                return true;
            }
        }

        resize();

        bool res = insert(id, title, price, quantity);

        return res;
    }

    Record* findById(const int32_t id, int& disk_reads) const
    {
        static Record res;
        disk_reads = 0;

        std::ifstream in(kDbFile, std::ios::binary);

        if (!in.is_open())
        {
            throw std::runtime_error("File for db didn't open to findById");
        }

        const int32_t ind = hash(id);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            in.seekg(kHeaderSize + ((ind + idx) % capacity_) * kRecordSize, std::ios::beg);
            in.read(reinterpret_cast<char*>(&record), kRecordSize);
            ++disk_reads;

            if (record.is_deleted == true && record.id == 0)
            {
                return nullptr;
            }

            if (!record.is_deleted && record.id == id)
            {
                res = record;
                return &res;
            }
        }

        return nullptr;
    }

    vector<Record> findByTitle(const string& title) const
    {
        return findBy(title, Fields::BY_TITLE);
    }

    vector<Record> findByQuantity(const int32_t quantity) const
    {
        return findBy(quantity, Fields::BY_QUANTITY);
    }

    vector<Record> findByPrice(const double price) const
    {
        return findBy(price, Fields::BY_PRICE);
    }

    bool deleteById(const int32_t id)
    {
        std::fstream file(kDbFile, std::ios::binary | std::ios::in | std::ios::out);

        if (!file.is_open())
        {
            throw std::runtime_error("Couldn't open database to delete by id");
        }

        const int32_t hash_id = hash(id);

        file.seekg(kHeaderSize, std::ios::beg);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            file.seekg(kHeaderSize + ((hash_id + idx) % capacity_) * kRecordSize, std::ios::beg);
            file.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (record.is_deleted && record.id == 0)
            {
                return false;
            }

            if (!record.is_deleted && record.id == id)
            {
                record.is_deleted = true;

                file.seekp(kHeaderSize + ((hash_id + idx) % capacity_) * kRecordSize, std::ios::beg);
                file.write(reinterpret_cast<char*>(&record), kRecordSize);

                --count_;
                writeHeader();
                return true;
            }
        }

        return false;
    }

    int32_t deleteByTitle(const std::string& title)
    {
        return deleteBy(title, Fields::BY_TITLE);
    }

    int32_t deleteByPrice(const double price)
    {
        return deleteBy(price, Fields::BY_PRICE);
    }

    int32_t deleteByQuantity(const int32_t quantity)
    {
        return deleteBy(quantity, Fields::BY_QUANTITY);
    }

    vector<Record> getAll() const
    {
        vector<Record> list;

        std::ifstream in(kDbFile, std::ios::binary);

        if (!in.is_open())
        {
            throw std::runtime_error("File for db didn't open to getAll");
        }

        in.seekg(kHeaderSize, std::ios::beg);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            in.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (record.is_deleted == false)
            {
                list.push_back(record);
            }
        }

        return list;
    }

    bool update(const int32_t id, const string& new_title, const double new_price, const int32_t new_quantity)
    {
        std::fstream file(kDbFile, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            return false;
        }

        const int32_t ind = hash(id);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            int32_t offset = kHeaderSize + ((ind + idx) % capacity_) * kRecordSize;

            file.seekg(offset, std::ios::beg);
            Record record;
            file.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (!record.is_deleted && record.id == id)
            {
                std::strncpy(record.title, new_title.c_str(), sizeof(record.title));
                record.title[sizeof(record.title) - 1] = '\0';
                record.price = new_price;
                record.quantity = new_quantity;

                file.seekp(offset, std::ios::beg);
                file.write(reinterpret_cast<char*>(&record), kRecordSize);

                return true;
            }

            if (record.is_deleted && record.id == 0)
            {
                return false;
            }
        }

        return false;
    }

    void clear()
    {
        createNew(100);
    }

    void backup() const
    {
        try
        {
            std::filesystem::copy_file(kDbFile, kBackupFile,
                                       std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Couldn't make backup database" << std::endl;
        }
    }

    void restore()
    {
        bool res = std::filesystem::copy_file(kBackupFile, kDbFile,
                                              std::filesystem::copy_options::overwrite_existing);

        if (res == false)
        {
            std::cerr << "Couldn't make restore" << std::endl;
        }

        std::ifstream in(kDbFile, std::ios::binary);

        Header header;

        in.read(reinterpret_cast<char*>(&header), kHeaderSize);

        capacity_ = header.capacity;
        count_ = header.count;
    }

    void exportCSV() const
    {
        std::ifstream in(kDbFile, std::ios::binary);

        if (!in)
        {
            throw std::runtime_error("Couldn't open database file in exportCSV");
        }

        std::ofstream out(kCSVFile);

        if (!out)
        {
            throw std::runtime_error("Couldn't open .csv file in exportCSV");
        }

        out << "id,title,price,quantity\n";

        in.seekg(kHeaderSize, std::ios::beg);

        for (int32_t idx = 0; idx < capacity_; ++idx)
        {
            Record record;
            in.read(reinterpret_cast<char*>(&record), kRecordSize);

            if (record.is_deleted)
            {
                continue;
            }

            std::string title(record.title);

            out << record.id << "," << '"' << title << '"' << "," << record.price << "," << record.quantity << "\n";
        }
    }
};

#endif
