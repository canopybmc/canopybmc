// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/ev_storage.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace chif;

class EvStorageTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        testDir_ = std::filesystem::temp_directory_path() / "chif_test_ev";
        std::filesystem::create_directories(testDir_);
        evPath_ = testDir_ / "evs.dat";
        storage_ = std::make_unique<EvStorage>(evPath_);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testDir_);
    }

    std::filesystem::path testDir_;
    std::filesystem::path evPath_;
    std::unique_ptr<EvStorage> storage_;
};

TEST_F(EvStorageTest, EmptyStorageOnStart)
{
    EXPECT_TRUE(storage_->load());
    EXPECT_EQ(storage_->count(), 0u);
    // File header (8 bytes) is always included in serialized size
    EXPECT_EQ(storage_->remainingSize(), EvStorage::maxSize() - 8);
}

TEST_F(EvStorageTest, SetAndGetByName)
{
    storage_->load();
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    EXPECT_TRUE(storage_->set("TESTEV", data));

    auto ev = storage_->getByName("TESTEV");
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->name, "TESTEV");
    EXPECT_EQ(ev->data, data);
}

TEST_F(EvStorageTest, SetAndGetByIndex)
{
    storage_->load();
    std::vector<uint8_t> data1 = {0x01};
    std::vector<uint8_t> data2 = {0x02};
    storage_->set("EV1", data1);
    storage_->set("EV2", data2);

    EXPECT_EQ(storage_->count(), 2u);

    auto ev0 = storage_->getByIndex(0);
    ASSERT_TRUE(ev0.has_value());
    EXPECT_EQ(ev0->name, "EV1");
    EXPECT_EQ(ev0->data, data1);

    auto ev1 = storage_->getByIndex(1);
    ASSERT_TRUE(ev1.has_value());
    EXPECT_EQ(ev1->name, "EV2");
    EXPECT_EQ(ev1->data, data2);

    auto ev2 = storage_->getByIndex(2);
    EXPECT_FALSE(ev2.has_value());
}

TEST_F(EvStorageTest, UpdateExisting)
{
    storage_->load();
    std::vector<uint8_t> data1 = {0x01};
    std::vector<uint8_t> data2 = {0x02, 0x03};

    storage_->set("TEST", data1);
    EXPECT_EQ(storage_->count(), 1u);

    storage_->set("TEST", data2);
    EXPECT_EQ(storage_->count(), 1u); // Still 1 entry

    auto ev = storage_->getByName("TEST");
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->data, data2);
}

TEST_F(EvStorageTest, DeleteByName)
{
    storage_->load();
    std::vector<uint8_t> d1 = {0x01};
    std::vector<uint8_t> d2 = {0x02};
    storage_->set("EV1", d1);
    storage_->set("EV2", d2);
    EXPECT_EQ(storage_->count(), 2u);

    EXPECT_TRUE(storage_->del("EV1"));
    EXPECT_EQ(storage_->count(), 1u);

    EXPECT_FALSE(storage_->getByName("EV1").has_value());
    EXPECT_TRUE(storage_->getByName("EV2").has_value());
}

TEST_F(EvStorageTest, DeleteNonexistent)
{
    storage_->load();
    EXPECT_FALSE(storage_->del("NOPE"));
}

TEST_F(EvStorageTest, DeleteAll)
{
    storage_->load();
    std::vector<uint8_t> d1 = {0x01};
    std::vector<uint8_t> d2 = {0x02};
    storage_->set("EV1", d1);
    storage_->set("EV2", d2);

    EXPECT_TRUE(storage_->deleteAll());
    EXPECT_EQ(storage_->count(), 0u);
}

TEST_F(EvStorageTest, GetByNameNotFound)
{
    storage_->load();
    EXPECT_FALSE(storage_->getByName("NOPE").has_value());
}

TEST_F(EvStorageTest, Persistence)
{
    // Write with first instance
    storage_->load();
    std::vector<uint8_t> data = {0xDE, 0xAD};
    storage_->set("PERSIST", data);

    // Create new instance, load from same file
    auto storage2 = std::make_unique<EvStorage>(evPath_);
    EXPECT_TRUE(storage2->load());
    EXPECT_EQ(storage2->count(), 1u);

    auto ev = storage2->getByName("PERSIST");
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->data, data);
}

TEST_F(EvStorageTest, Statistics)
{
    storage_->load();

    EXPECT_EQ(storage_->count(), 0u);
    // File header (8 bytes) is always included in serialized size
    EXPECT_EQ(storage_->remainingSize(), EvStorage::maxSize() - 8);

    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    storage_->set("TEST", data);

    EXPECT_EQ(storage_->count(), 1u);
    // Header (8) + entry (32 + 2 + 3) = 45 bytes used
    EXPECT_EQ(storage_->remainingSize(), EvStorage::maxSize() - 45);
}

TEST_F(EvStorageTest, EmptyName)
{
    storage_->load();
    std::vector<uint8_t> data = {0x01};
    EXPECT_FALSE(storage_->set("", data));
}

TEST_F(EvStorageTest, MaxDataSize)
{
    storage_->load();
    std::vector<uint8_t> data(maxEvDataSize, 0xFF);
    EXPECT_TRUE(storage_->set("BIG", data));

    auto ev = storage_->getByName("BIG");
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->data.size(), maxEvDataSize);
}

TEST_F(EvStorageTest, OverMaxDataSizeFails)
{
    storage_->load();
    std::vector<uint8_t> tooLarge(maxEvDataSize + 1, 0xFF);
    EXPECT_FALSE(storage_->set("TOO_BIG", tooLarge));
}

TEST_F(EvStorageTest, EmptyData)
{
    storage_->load();
    std::vector<uint8_t> empty;
    EXPECT_TRUE(storage_->set("EMPTY", empty));

    auto ev = storage_->getByName("EMPTY");
    ASSERT_TRUE(ev.has_value());
    EXPECT_TRUE(ev->data.empty());
}

TEST_F(EvStorageTest, CorruptedMagicRejected)
{
    std::ofstream out(evPath_, std::ios::binary);
    uint32_t badMagic = 0xDEADBEEF;
    uint32_t cnt = 0;
    out.write(reinterpret_cast<const char*>(&badMagic), sizeof(badMagic));
    out.write(reinterpret_cast<const char*>(&cnt), sizeof(cnt));
    out.close();

    EXPECT_FALSE(storage_->load());
    EXPECT_EQ(storage_->count(), 0u);
}

TEST_F(EvStorageTest, PersistenceAfterDelete)
{
    storage_->load();
    std::vector<uint8_t> d1 = {0x01};
    std::vector<uint8_t> d2 = {0x02};
    storage_->set("EV1", d1);
    storage_->set("EV2", d2);
    storage_->del("EV1");

    auto storage2 = std::make_unique<EvStorage>(evPath_);
    storage2->load();
    EXPECT_EQ(storage2->count(), 1u);
    EXPECT_FALSE(storage2->getByName("EV1").has_value());
    EXPECT_TRUE(storage2->getByName("EV2").has_value());
}

TEST_F(EvStorageTest, MultipleEntries)
{
    storage_->load();
    for (int i = 0; i < 50; i++)
    {
        std::string name = "EV" + std::to_string(i);
        std::vector<uint8_t> data(10, static_cast<uint8_t>(i));
        EXPECT_TRUE(storage_->set(name, data));
    }
    EXPECT_EQ(storage_->count(), 50u);

    auto storage2 = std::make_unique<EvStorage>(evPath_);
    storage2->load();
    EXPECT_EQ(storage2->count(), 50u);

    auto ev = storage2->getByName("EV25");
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->data.size(), 10u);
    EXPECT_EQ(ev->data[0], 25);
}
