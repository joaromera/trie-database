#include "Database.h"
#include <list>
#include <tuple>
#include <algorithm>

using namespace Db::Types;

void Database::createTable(const std::string &name,
  const linear_set<std::string> &keys,
  const std::vector<std::string> &columns,
  const std::vector<Datum> &types)
{
  mTableNames.fast_insert(name);
  mTables.insert(make_pair(name, Table(keys, columns, types)));
}

void Database::addRecord(const Record &record, const std::string &tableName)
{
  Table &t = mTables.at(tableName);
  t.addRecord(record);
  for (const auto& c : record.columns())
  {
    if (hasIndex(tableName, c))
    {
      mIndexRefs[tableName][c][record.value(c)].insert(record);
    }
  }
}

const linear_set<std::string> &Database::tableNames() const
{
  return mTableNames;
}

const Table &Database::getTable(const std::string &name) const
{
  return mTables.at(name);
}

int Database::filterUsageCount(const Database::Filters &filter) const
{
  if (mFilterUsageCount.count(filter)) return mFilterUsageCount.at(filter);

  return 0;
}

bool Database::isValidRecord(const Record &record, const std::string &name) const
{
  const Table &t = mTables.at(name);

  if (t.columns().size() != record.columns().size())
    return false;

  for (const auto& c : record.columnValues())
  {
    if (t.columns().count(c.first) == 0)
      return false;

    if (t.columnType(c.first).isInteger() != c.second.isInteger())
      return false;
  }

  for (const auto& rt : t.records())
  {
    int coincidencias = 0;
    int claves_en_t = t.keys().size();

    for (const auto& c : t.keys())
    {
      if (record.value(c) == rt.value(c))
        coincidencias++;
    }

    if (claves_en_t - coincidencias == 0)
      return false;
  }

  return true;
}

std::list<Record> &Database::filterRecords(
  const std::string &column,
  const Datum &value,
  std::list<Record> &records,
  bool equals) const
{
  auto iter = records.begin();

  while (iter != records.end())
  {
    auto now = iter++;
    const bool excludeNonMatch = (now->value(column) != value) && equals;
    const bool excludeMatch = (now->value(column) == value) && !equals;

    if (excludeNonMatch || excludeMatch)
    {
      records.erase(now);
    }
  }

  return records;
}

std::pair<std::vector<std::string>, std::vector<Datum>>Database::tableTypes(const Table &t)
{
  std::vector<std::string> columns;
  std::vector<Datum> types;
  for (const auto& c : t.columns())
  {
    columns.push_back(c);
    types.push_back(t.columnType(c));
  }
  return { columns, types };
}

bool Database::isValidFilter(const Filters &filters, const std::string &name) const
{
  const Table &t = mTables.at(name);
  for (const auto &filter : filters)
  {
    const auto &column = filter.column();
    const auto &datum = filter.datum();

    if (!t.columns().count(column))
      return false;

    if (t.columnType(column).isInteger() != datum.isInteger())
      return false;
  }
  return true;
}

Table Database::search(const Database::Filters &filters, const std::string &name)
{
  updateFilterUsageCount(filters);

  const Table &tableToFilter = getTable(name);
  auto filterTableTypes = tableTypes(tableToFilter);

  std::list<Record> records(tableToFilter.records().begin(), tableToFilter.records().end());

  for (const auto& filter : filters)
  {
    filterRecords(filter.column(), filter.datum(), records, filter.equals());
  }

  Table t(tableToFilter.keys(), filterTableTypes.first, filterTableTypes.second);

  for (const auto& r : records)
  {
    t.addRecord(r);
  }

  return t;
}
void Database::updateFilterUsageCount(const Database::Filters &filters)
{
  if (mFilterUsageCount.count(filters))
  {
    mFilterUsageCount.at(filters)++;
  }
  else
  {
    mFilterUsageCount.fast_insert({ filters, 1 });
  }
}

linear_set<Database::Filters> Database::mostUsedFilter() const
{
  linear_set<Filters> mostUsedFilters;
  int max = 0;
  for (const auto&filter : mFilterUsageCount)
  {
    if (filter.second >= max)
    {
      if (filter.second > max)
      {
        mostUsedFilters = linear_set<Filters>();
        max = filter.second;
      }
      mostUsedFilters.fast_insert(filter.first);
    }
  }
  return mostUsedFilters;
}

void Database::createIndex(const std::string &table, const std::string &column)
{
  const linear_set<Record> &records = getTable(table).records();

  for (const auto& r : records)
  {
    mIndexRefs[table][column][r.value(column)].insert(r);
  }
}

bool Database::hasIndex(const std::string &table, const std::string &column)
{
  return mIndexRefs[table].count(column) >= 1;
}

join_iterator Database::join(const std::string &table1,
  const std::string &table2,
  const std::string &columnName)
{
  const bool firstTableHasIndex = hasIndex(table1, columnName);
  const auto &indexedTable = firstTableHasIndex ? table1 : table2;
  const auto &nonIndexedTable = firstTableHasIndex ? table2 : table1;

  return join_helper(indexedTable, nonIndexedTable, columnName, firstTableHasIndex);
}

join_iterator Database::join_helper(const std::string &leftTable,
  const std::string &rightTable,
  const std::string &joinColumn,
  const bool &order)
{
  const Table &t2 = getTable(rightTable);
  auto it2 = t2.begin();
  auto table2Size = t2.size();
  auto valueAtIt2 = it2->value(joinColumn);

  auto it = mIndexRefs[leftTable].at(joinColumn).find(valueAtIt2)->second.begin();
  auto it_end = mIndexRefs[leftTable].at(joinColumn).find(valueAtIt2)->second.end();

  // Busca primer coincidencia entre las dos tableNames
  while (table2Size != 0 && it == it_end)
  {
    valueAtIt2 = it2->value(joinColumn);
    it = mIndexRefs[leftTable].at(joinColumn).find(valueAtIt2)->second.begin();
    it_end = mIndexRefs[leftTable].at(joinColumn).find(valueAtIt2)->second.end();
    if (it++ == it_end) { table2Size--; }
  }

  if (table2Size == 0) return join_iterator();

  auto &tab = mIndexRefs[leftTable];
  auto &col = tab.at(joinColumn);
  if (col.find(valueAtIt2) == col.end()) return join_iterator();

  auto &dat = col.at(valueAtIt2);
  auto it_tabla_con_indice = dat.begin();
  auto it_tabla_sin_indice = t2.begin();
  auto diccClaves = std::make_shared<std::map<Datum, linear_set<Record>>>(mIndexRefs[leftTable].at(joinColumn));
  auto cant_reg_por_indice = mIndexRefs[leftTable].at(joinColumn).at(valueAtIt2).size();

  return join_iterator(it_tabla_con_indice, it_tabla_sin_indice, cant_reg_por_indice, table2Size, diccClaves, joinColumn, order);
}

join_iterator Database::join_end()
{
  return join_iterator();
}
