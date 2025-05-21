 #pragma once
 #include "db/DbConnectionPool.h"
 
#include <string>

namespace http
{

class MysqlUtil
{
public:
    static void init(const std::string& host, const std::string& user,
                    const std::string& password, const std::string& database,
                    size_t poolSize = 10)
    {
        http::db::DbConnectionPool::getInstance().init(
            host, user, password, database, poolSize);
    }

    template<typename... Args>
    sql::ResultSet* executeQuery(const std::string& sql, Args&&... args)
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->executeQuery(sql, std::forward<Args>(args)...);
    }

    template<typename... Args>
    int executeUpdate(const std::string& sql, Args&&... args)
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->executeUpdate(sql, std::forward<Args>(args)...);
    }


    //实现以c++风格访问数据库中的内容
    std::vector<std::map<std::string, std::string>> executeQueryAsVector(const std::string& sql) {
        std::vector<std::map<std::string, std::string>> rows;

        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        std::unique_ptr<sql::ResultSet> resultSet(conn->executeQuery(sql));

        sql::ResultSetMetaData* meta = resultSet->getMetaData();
        int columnCount = meta->getColumnCount();

        while (resultSet->next()) {
            std::map<std::string, std::string> row;
            for (int i = 1; i <= columnCount; ++i) {
                std::string columnName = meta->getColumnLabel(i);  // 推荐用 getColumnLabel 而不是 getColumnName
                std::string value = resultSet->getString(i);
                row[columnName] = value;
            }
            rows.push_back(std::move(row));
        }
    return rows;
}

};

} // namespace http
