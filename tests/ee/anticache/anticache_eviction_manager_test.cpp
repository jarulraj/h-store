/* Copyright (C) 2012 by H-Store Project
 * Brown University
 * Massachusetts Institute of Technology
 * Yale University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "harness.h"
#include "common/TupleSchema.h"
#include "common/types.h"
#include "common/NValue.hpp"
#include "common/ValueFactory.hpp"
#include "common/ValuePeeker.hpp"
#include "execution/VoltDBEngine.h"
#include "storage/persistenttable.h"
#include "storage/tablefactory.h"
#include "storage/tableutil.h"
#include "indexes/tableindex.h"
#include "storage/tableiterator.h"
#include "storage/CopyOnWriteIterator.h"
#include "common/DefaultTupleSerializer.h"
#include <vector>
#include <string>
#include <stdint.h>
#include <set>
#include <stdlib.h>
#include <time.h>
#include "boost/scoped_ptr.hpp"

#include "anticache/AntiCacheDB.h"

#define BLOCK_SIZE 1024000
#define MAX_SIZE 1024000000


using namespace std;
using namespace voltdb;
using stupidunit::ChTempDir;

/**
 * AntiCacheDB Tests
 */
class AntiCacheEvictionManagerTest : public Test {
public:
    AntiCacheEvictionManagerTest() {
        ChTempDir tempdir;

        m_tuplesInserted = 0;
        m_tuplesUpdated = 0;
        m_tuplesDeleted = 0;

        m_engine = new voltdb::VoltDBEngine();
        m_engine->initialize(1,1, 0, 0, "");
        //string tmpdir_name = tempdir.name();
        //string nvmdir = "/tmp/nvm";
        //string berkdir = ".";
        //m_engine->antiCacheInitialize(tmpdir_name, ANTICACHEDB_NVM, BLOCK_SIZE, MAX_SIZE);
        //m_engine->antiCacheAddDB(tmpdir_name, ANTICACHEDB_BERKELEY, BLOCK_SIZE, MAX_SIZE);

        //ExecutorContext* ctx = m_engine->getExecutorContext();

        //ctx->addAntiCacheDB(tmpdir_name, BLOCK_SIZE, ANTICACHEDB_BERKELEY, MAX_SIZE);


        
        m_columnNames.push_back("1");
        m_columnNames.push_back("2");
        
        m_tableSchemaTypes.push_back(voltdb::VALUE_TYPE_INTEGER);
        m_primaryKeyIndexSchemaTypes.push_back(voltdb::VALUE_TYPE_INTEGER);
        m_tableSchemaTypes.push_back(voltdb::VALUE_TYPE_INTEGER);
        
        m_tableSchemaColumnSizes.push_back(NValue::getTupleStorageSize(voltdb::VALUE_TYPE_INTEGER));
        m_primaryKeyIndexSchemaColumnSizes.push_back(voltdb::VALUE_TYPE_INTEGER);
        m_tableSchemaColumnSizes.push_back(NValue::getTupleStorageSize(voltdb::VALUE_TYPE_INTEGER));
        m_primaryKeyIndexSchemaColumnSizes.push_back(voltdb::VALUE_TYPE_INTEGER);
        
        m_tableSchemaAllowNull.push_back(false);
        m_primaryKeyIndexSchemaAllowNull.push_back(false);
        m_tableSchemaAllowNull.push_back(false);
        m_primaryKeyIndexSchemaAllowNull.push_back(false);
        
        m_primaryKeyIndexColumns.push_back(0);
        
    };
   
    ~AntiCacheEvictionManagerTest() {
        delete m_engine;
        //delete m_table;
        voltdb::TupleSchema::freeTupleSchema(m_primaryKeyIndexSchema);
    }
    
    
    void initTable(bool allowInlineStrings) {
        m_tableSchema = voltdb::TupleSchema::createTupleSchema(m_tableSchemaTypes,
                                                               m_tableSchemaColumnSizes,
                                                               m_tableSchemaAllowNull,
                                                               allowInlineStrings);
        
        m_primaryKeyIndexSchema = voltdb::TupleSchema::createTupleSchema(m_primaryKeyIndexSchemaTypes,
                                                                         m_primaryKeyIndexSchemaColumnSizes,
                                                                         m_primaryKeyIndexSchemaAllowNull,
                                                                         allowInlineStrings);
        voltdb::TableIndexScheme indexScheme = voltdb::TableIndexScheme("primaryKeyIndex",
                                                                         voltdb::BALANCED_TREE_INDEX,
                                                                         m_primaryKeyIndexColumns,
                                                                         m_primaryKeyIndexSchemaTypes,
                                                                         true, false, m_tableSchema);

//           voltdb::TableIndexScheme indexScheme = voltdb::TableIndexScheme("primaryKeyIndex",
//                                                                        voltdb::HASH_TABLE_INDEX,
//                                                                        m_primaryKeyIndexColumns,
//                                                                        m_primaryKeyIndexSchemaTypes,
//                                                                        true, false, m_tableSchema);
        indexScheme.keySchema = m_primaryKeyIndexSchema;
        std::vector<voltdb::TableIndexScheme> indexes;
        
        m_table = dynamic_cast<voltdb::PersistentTable*>(voltdb::TableFactory::getPersistentTable
                                                         (0, m_engine->getExecutorContext(), "Foo",
                                                          m_tableSchema, &m_columnNames[0], indexScheme, indexes, 0,
                                                          false, false));
                
        TupleSchema *evictedSchema = TupleSchema::createEvictedTupleSchema();
                
        // Get the column names for the EvictedTable
        std::string evictedColumnNames[2];
        evictedColumnNames[0] = std::string("BLOCK_ID");
        evictedColumnNames[1] = std::string("TUPLE_OFFSET");
        
        voltdb::Table *evicted_table = TableFactory::getEvictedTable(
                                                                     0,
                                                                     m_engine->getExecutorContext(),
                                                                     "Foo_EVICTED",
                                                                     evictedSchema,
                                                                     &evictedColumnNames[0]);
        
        m_table->setEvictedTable(evicted_table);
    }
    
    void cleanupTable()
    {
        //printf("delete from cleanupTable(): %p\n", m_table->getEvictedTable());
        delete m_table->getEvictedTable();
        delete m_table;
    
    }
    
    voltdb::VoltDBEngine *m_engine;
    voltdb::TupleSchema *m_tableSchema;
    voltdb::TupleSchema *m_primaryKeyIndexSchema;
    voltdb::PersistentTable *m_table;
    std::vector<std::string> m_columnNames;
    std::vector<voltdb::ValueType> m_tableSchemaTypes;
    std::vector<int32_t> m_tableSchemaColumnSizes;
    std::vector<bool> m_tableSchemaAllowNull;
    std::vector<voltdb::ValueType> m_primaryKeyIndexSchemaTypes;
    std::vector<int32_t> m_primaryKeyIndexSchemaColumnSizes;
    std::vector<bool> m_primaryKeyIndexSchemaAllowNull;
    std::vector<int> m_primaryKeyIndexColumns;

    ChTempDir tempdir;
    
    int32_t m_tuplesInserted;
    int32_t m_tuplesUpdated;
    int32_t m_tuplesDeleted;
    
};

TEST_F(AntiCacheEvictionManagerTest, MigrateBlock) {
    ChTempDir tempdir;
    //ChTempDir tempdir2;

    string temp = tempdir.name();

    ExecutorContext* ctx = m_engine->getExecutorContext();
    //ctx->enableAntiCache(m_engine, temp, BLOCK_SIZE, ANTICACHEDB_NVM, MAX_SIZE);
    //ctx->addAntiCacheDB(temp, BLOCK_SIZE, ANTICACHEDB_BERKELEY, MAX_SIZE);

    AntiCacheEvictionManager* acem = new AntiCacheEvictionManager(m_engine);
    //AntiCacheEvictionManager* acem = ctx->getAntiCacheEvictionManager();
    //AntiCacheDB* nvmdb = ctx->getAntiCacheDB(0);
    //AntiCacheDB* berkeleydb = ctx->getAntiCacheDB(1);
    AntiCacheDB* nvmdb = new NVMAntiCacheDB(ctx, temp, BLOCK_SIZE, MAX_SIZE);
    AntiCacheDB* berkeleydb = new BerkeleyAntiCacheDB(ctx, temp, BLOCK_SIZE, MAX_SIZE);

    int16_t nvm_acid = acem->addAntiCacheDB(nvmdb);
    int16_t berkeley_acid = acem->addAntiCacheDB(berkeleydb);
    
    ASSERT_EQ(nvm_acid, nvmdb->getACID());
    ASSERT_EQ(berkeley_acid, berkeleydb->getACID());
    ASSERT_EQ(0, nvm_acid);
    ASSERT_EQ(1, berkeley_acid);

    string tableName("TEST");
    string payload("Test payload");

    int16_t blockId = nvmdb->nextBlockId();
    nvmdb->writeBlock(tableName,
        blockId,
        1,
        const_cast<char*>(payload.data()),
        static_cast<int>(payload.size())+1);

    //int32_t fullBlockId = (nvm_acid << 16) | blockId;
    //VOLT_INFO("acid: %x, blockId: %x fullBlockId: %x\n", nvm_acid, blockId, fullBlockId);
    
    //AntiCacheBlock* nvmblock = nvmdb->readBlock(blockId);
    int32_t newBlockId = (int32_t) acem->migrateBlock(blockId, berkeleydb);
    int16_t _new_block_id = (int16_t) (newBlockId & 0x0000FFFF);
    VOLT_INFO("blockId: %x newBlockId: %x _new_block_id: %x\n", blockId, newBlockId, _new_block_id);
    AntiCacheBlock* berkeleyblock = berkeleydb->readBlock(_new_block_id);
    VOLT_INFO("tableName: %s berkeleyblock name: %s\n", tableName.c_str(), berkeleyblock->getTableName().c_str());
    
    //ASSERT_EQ(blockId, nvmblock->getBlockId());
    ASSERT_EQ(_new_block_id, berkeleyblock->getBlockId());
    //ASSERT_EQ(payload.size()+1, nvmblock->getSize());
    ASSERT_EQ(payload.size()+1, berkeleyblock->getSize());
    //ASSERT_EQ(tableName, nvmblock->getTableName());
    ASSERT_EQ(0, tableName.compare(berkeleyblock->getTableName()));
    //ASSERT_EQ(0, payload.compare(nvmblock->ge"tData()));
    ASSERT_EQ(0, payload.compare(berkeleyblock->getData()));
    
    delete berkeleyblock;

    string tableNameLRU("LRU Table");
    string payloadLRU("LRU Test payload");
    
    int16_t blockIdLRU = nvmdb->nextBlockId();
    nvmdb->writeBlock(tableNameLRU,
        blockIdLRU,
        1,
        const_cast<char*>(payloadLRU.data()),
        static_cast<int>(payloadLRU.size())+1);

    blockId = nvmdb->nextBlockId();
    nvmdb->writeBlock(tableName,
        blockId,
        1,
        const_cast<char*>(payload.data()),
        static_cast<int>(payload.size())+1);
    
    //AntiCacheBlock* nvmblock = nvmdb->readBlock(blockId);
    newBlockId = (int32_t)acem->migrateLRUBlock(nvmdb, berkeleydb);
    
    _new_block_id = (int16_t) (newBlockId & 0x0000FFFF);

    berkeleyblock = berkeleydb->readBlock(_new_block_id);
    //VOLT_INFO("tableName: %s berkeleyblock name: %s\n", tableName.c_str(), berkeleyblock->getTableName().c_str());
    
    //ASSERT_EQ(blockId, nvmblock->getBlockId());
    ASSERT_EQ(_new_block_id, berkeleyblock->getBlockId());
    //ASSERT_EQ(payload.size()+1, nvmblock->getSize());
    ASSERT_EQ(payloadLRU.size()+1, berkeleyblock->getSize());
    //ASSERT_EQ(tableName, nvmblock->getTableName());
    ASSERT_EQ(0, tableNameLRU.compare(berkeleyblock->getTableName()));
    //ASSERT_EQ(0, payload.compare(nvmblock->getData()));
    ASSERT_EQ(0, payloadLRU.compare(berkeleyblock->getData()));
    
 
    //delete ctx;
    //delete nvmblock;
    delete berkeleyblock;
    delete berkeleydb;
    VOLT_INFO("BerkeleyDB deleted\n");
    delete nvmdb;
    delete acem;
}


#ifndef ANTICACHE_TIMESTAMPS
TEST_F(AntiCacheEvictionManagerTest, GetTupleID)
{
    initTable(true); 
    
    TableTuple tuple = m_table->tempTuple();
    
    tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
    tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
    m_table->insertTuple(tuple);
    
    // get the tuple that was just inserted
    tuple = m_table->lookupTuple(tuple); 
    
    int tuple_id = m_table->getTupleID(tuple.address()); 
    
    //ASSERT_NE(tuple_id, -1); 
    ASSERT_EQ(tuple_id, 0);
    
    cleanupTable(); 
}

TEST_F(AntiCacheEvictionManagerTest, NewestTupleID)
{
    int inserted_tuple_id, newest_tuple_id; 
    
    initTable(true); 
    
    TableTuple tuple = m_table->tempTuple();
    
    tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
    tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
    m_table->insertTuple(tuple);
    
    // get the tuple that was just inserted
    tuple = m_table->lookupTuple(tuple); 
    
    inserted_tuple_id = m_table->getTupleID(tuple.address()); 
    newest_tuple_id = m_table->getNewestTupleID(); 
    
    ASSERT_EQ(inserted_tuple_id, newest_tuple_id);
    
    cleanupTable();
}

TEST_F(AntiCacheEvictionManagerTest, OldestTupleID)
{
    int inserted_tuple_id, oldest_tuple_id; 
    
    initTable(true); 
    
    TableTuple tuple = m_table->tempTuple();
    
    tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
    tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
    m_table->insertTuple(tuple);
    
    // get the tuple that was just inserted
    tuple = m_table->lookupTuple(tuple); 
    
    inserted_tuple_id = m_table->getTupleID(tuple.address()); 
    oldest_tuple_id = m_table->getOldestTupleID(); 
    
    ASSERT_EQ(inserted_tuple_id, oldest_tuple_id);
    
    cleanupTable();
}



TEST_F(AntiCacheEvictionManagerTest, InsertMultipleTuples)
{
    int num_tuples = 10; 

    initTable(true); 
    
    TableTuple tuple = m_table->tempTuple();
    
    uint32_t oldest_tuple_id, newest_tuple_id; 
    
    for(int i = 0; i < num_tuples; i++) // insert 10 tuples
    {
        tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
        tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
        m_table->insertTuple(tuple);
        
                tuple = m_table->lookupTuple(tuple);

        if(i == 0)
        {
            oldest_tuple_id = m_table->getTupleID(tuple.address()); 
        }
        else if(i == num_tuples-1)
        {
            newest_tuple_id = m_table->getTupleID(tuple.address()); 
        }
    }
        
    ASSERT_EQ(num_tuples, m_table->getNumTuplesInEvictionChain()); 
    ASSERT_EQ(oldest_tuple_id, m_table->getOldestTupleID());
    ASSERT_EQ(newest_tuple_id, m_table->getNewestTupleID());
    
    cleanupTable();
}

TEST_F(AntiCacheEvictionManagerTest, DeleteSingleTuple)
{
    initTable(true); 
    
    TableTuple tuple = m_table->tempTuple(); 
    
    tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
    tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
    
    m_table->insertTuple(tuple);
    
    ASSERT_EQ(1, m_table->getNumTuplesInEvictionChain()); 
    
    tuple = m_table->lookupTuple(tuple);
    m_table->deleteTuple(tuple, true);
    
    ASSERT_EQ(0, m_table->getNumTuplesInEvictionChain());
    
    cleanupTable();
}

TEST_F(AntiCacheEvictionManagerTest, DeleteMultipleTuples)
{
    int num_tuples = 100; 

    initTable(true); 
      
    TableTuple tuple = m_table->tempTuple();
        
    for(int i = 0; i < num_tuples; i++) // insert 10 tuples
    {
        tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
        tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
        m_table->insertTuple(tuple);
    }
    
    VOLT_INFO("%d == %d", num_tuples, m_table->getNumTuplesInEvictionChain());
    ASSERT_EQ(num_tuples, m_table->getNumTuplesInEvictionChain()); 
        
        int num_tuples_deleted = 0; 
        TableIterator itr(m_table); 
        while(itr.hasNext())
        {
                itr.next(tuple); 
                
                if(rand() % 2 == 0)  // delete each tuple with probability .5
                {
            m_table->deleteTuple(tuple, true); 
            ++num_tuples_deleted; 
                }
        }
        

        ASSERT_EQ((num_tuples - num_tuples_deleted), m_table->getNumTuplesInEvictionChain());
    
    cleanupTable();
}

TEST_F(AntiCacheEvictionManagerTest, UpdateIndexPerformance)
{
    int num_tuples = 100000;
    int num_index_updates = 8;
    
    struct timeval start, end;
    
    long  seconds, useconds;
    double mtime; 

    initTable(true); 
      
    TableTuple tuple = m_table->tempTuple();

    int iterations = 0;
    
    for(int i = 0; i < num_tuples; i++) // insert tuples
    {
        tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
        tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
        m_table->insertTuple(tuple);
    }
    
    for(int i = 0; i < num_index_updates; i++)
    {
        TableIterator itr1(m_table);
        iterations = 0; 
        gettimeofday(&start, NULL);
        while(itr1.hasNext())
        {
            itr1.next(tuple);
            for(int j = 0; j < i+1; j++)
            {
                m_table->setEntryToNewAddressForAllIndexes(&tuple, NULL);
            }
            
            if(++iterations == 1000)
                break; 
        }
        gettimeofday(&end, NULL);
        
        seconds  = end.tv_sec  - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;
        mtime = (double)((seconds) * 1000 + useconds/1000);
        
        VOLT_INFO("total time for 1000 index updates: %f milliseconds", mtime);
    }
    
    cleanupTable();
}

//TEST_F(AntiCacheEvictionManagerTest, EvictBlockPerformanceTest)
//{
//    int num_tuples = 100000;
//    int block_size = 1048576; // 1 MB
//    
//    initTable(true);
//    
//    TableTuple tuple = m_table->tempTuple();
//    
//      time_t start;
//      time_t stop;
//    
//    for(int i = 0; i < num_tuples; i++) // insert tuples
//    {
//        tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
//        tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
//        m_table->insertTuple(tuple);
//    }
//    
//      TableIterator itr(m_table);
//      time(&start);
//      m_table->evictBlockToDisk(block_size, 1);
//      time(&stop);
//      
////    double seconds = difftime(stop, start);
////    VOLT_INFO("total time: %f seconds", seconds);
//    
//    cleanupTable();
//}

// TEST_F(AntiCacheEvictionManagerTest, UpdateTuple)
// {
//   
//     int num_tuples = 0; 
// 
//     initTable(true); 
//     
//     TableTuple tuple = m_table->tempTuple();
//     
//     int oldest_tuple_id, newest_tuple_id; 
//     
//     for(int i = 0; i < num_tuples; i++) // insert 10 tuples
//     {
//         tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
//         tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
//         m_table->insertTuple(tuple);
//         
//     }
//          
//     oldest_tuple_id = m_table->getOldestTupleID(); 
//     tuple.move(m_table->dataPtrForTuple(oldest_tuple_id)); 
//     
//     // create a new tuple from the old tuple and update its non-key value
//     TableTuple updated_tuple(tuple); 
//     updated_tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
//     
//     m_table.updateTuple(&tuple, &updated_tuple, false); 
//     
//     // the oldest tuple was updated, so should now be the newest
//     ASSERT_EQ(oldest_tuple_id, m_table->getNewestTupleID()); 
//   
// }
#else
TEST_F(AntiCacheEvictionManagerTest, GetTupleTimeStamp)
{
    initTable(true); 
    
    TableTuple tuple = m_table->tempTuple();
    
    tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
    tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
    m_table->insertTuple(tuple);
    
    // get the tuple that was just inserted
    tuple = m_table->lookupTuple(tuple); 
    
    uint32_t time_stamp = tuple.getTimeStamp();

    ASSERT_NE(time_stamp, 0);
    
    cleanupTable(); 
}

TEST_F(AntiCacheEvictionManagerTest, TestEvictionOrder)
{
    int num_tuples = 100; 

    initTable(true); 
      
    TableTuple tuple = m_table->tempTuple();
    int tuple_size = m_tableSchema->tupleLength() + TUPLE_HEADER_SIZE;

        
    for(int i = 0; i < num_tuples; i++) // insert 10 tuples
    {
        tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
        tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
        m_table->insertTuple(tuple);
    }
    
    EvictionIterator itr(m_table); 

    itr.reserve(20 * tuple_size);

    ASSERT_TRUE(itr.hasNext());

    uint32_t oldTimeStamp = 0;

    while(itr.hasNext()) {
        itr.next(tuple); 
        
        uint32_t newTimeStamp = tuple.getTimeStamp();
        ASSERT_LE(oldTimeStamp, newTimeStamp);
        oldTimeStamp = newTimeStamp;
    }

    cleanupTable();
}

// still couldn't pass
TEST_F(AntiCacheEvictionManagerTest, UpdateIndexPerformance)
{
    int num_tuples = 100000;
    int num_index_updates = 8;

    struct timeval start, end;

    long  seconds, useconds;
    double mtime; 

    initTable(true); 

    TableTuple tuple = m_table->tempTuple();

    int iterations = 0;

    for(int i = 0; i < num_tuples; i++) // insert tuples
    {
        tuple.setNValue(0, ValueFactory::getIntegerValue(m_tuplesInserted++));
        tuple.setNValue(1, ValueFactory::getIntegerValue(rand()));
        m_table->insertTuple(tuple);
    }

    for(int i = 0; i < num_index_updates; i++)
    {
        TableIterator itr1(m_table);
        iterations = 0; 
        gettimeofday(&start, NULL);
        while(itr1.hasNext())
        {
            itr1.next(tuple);
            for(int j = 0; j < i+1; j++)
            {
                m_table->setEntryToNewAddressForAllIndexes(&tuple, NULL);
            }

            if(++iterations == 1000)
                break; 
        }
        gettimeofday(&end, NULL);

        seconds  = end.tv_sec  - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;
        mtime = (double)((seconds) * 1000 + useconds/1000);

        VOLT_INFO("total time for 1000 index updates: %f milliseconds", mtime);
    }

    cleanupTable();
}

#endif

int main() {
    return TestSuite::globalInstance()->runAll();
}

