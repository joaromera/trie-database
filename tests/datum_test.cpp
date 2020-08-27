#include "gtest/gtest.h"
#include "../src/Datum.h"

TEST(dato_test, generadores)
{
  Datum(5);
  Datum("hola");
}

TEST(dato_test, esNat)
{
  EXPECT_EQ(Datum(5).isInteger(), true);
  EXPECT_EQ(Datum(100).isInteger(), true);
  EXPECT_EQ(Datum("").isInteger(), false);
  EXPECT_EQ(Datum("hola").isInteger(), false);
}

TEST(dato_test, esString)
{
  EXPECT_EQ(Datum("hola").isString(), true);
  EXPECT_EQ(Datum("chau").isString(), true);
  EXPECT_EQ(Datum(10).isString(), false);
  EXPECT_EQ(Datum(5).isString(), false);
}

TEST(dato_test, valores)
{
  EXPECT_EQ(Datum(10).value<int>(), 10);
  EXPECT_EQ(Datum(5).value<int>(), 5);
  EXPECT_EQ(Datum(100).value<int>(), 100);
  EXPECT_EQ(Datum("").value<std::string>(), "");
  EXPECT_EQ(Datum("hola").value<std::string>(), "hola");
  EXPECT_EQ(Datum("chau").value<std::string>(), "chau");
}

TEST(dato_test, igobs)
{
  EXPECT_EQ(Datum(5), Datum(5));
  EXPECT_NE(Datum(10), Datum(5));

  EXPECT_EQ(Datum("hola"), Datum("hola"));
  EXPECT_NE(Datum("holas"), Datum("hola"));
}

TEST(dato_test, shortcuts)
{
  EXPECT_EQ(Datum{ 5 }, Datum{ 5 });
  EXPECT_EQ(Datum("hola"), Datum("hola"));
}

TEST(dato_test, implicit_cast)
{
  EXPECT_EQ(Datum("Hola"), (Datum) "Hola");
}