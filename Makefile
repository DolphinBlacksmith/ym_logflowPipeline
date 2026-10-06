# LogFlow - Yazılım Mimarisi Dönem Projesi (Artım 2)
# Derleyici ve Bayraklar
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
COVFLAGS = -Wall -Wextra -std=c++17 -O0 -g --coverage -fprofile-arcs -ftest-coverage

# Hedefler
TARGET = logflow
LEGACY_TARGET = pipeline
TEST_TARGET = test_runner
COV_TARGET = test_runner_cov

SRCS = main.cpp
TEST_SRCS = tests/test_runner.cpp

all: $(TARGET) $(LEGACY_TARGET)

$(TARGET): $(SRCS) LogRecord.hpp ParserStage.hpp ConsoleSink.hpp Pipeline.hpp FileLineSource.hpp Source.hpp Sink.hpp Stage.hpp Emitter.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

$(LEGACY_TARGET): $(TARGET)
	ln -sf $(TARGET) $(LEGACY_TARGET)

run: $(TARGET)
	./$(TARGET) data/access-small.log

run-malformed: $(TARGET)
	./$(TARGET) data/access-malformed.log

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) LogRecord.hpp ParserStage.hpp CollectingEmitter.hpp
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRCS)

coverage: clean
	@echo "Coverage derlemesi yapiliyor..."
	$(CXX) $(COVFLAGS) -o $(COV_TARGET) $(TEST_SRCS)
	@echo "Testler kosuluyor..."
	./$(COV_TARGET)
	@echo ""
	@echo "============================================================"
	@echo " GCOV SATIR KAPSAMA (LINE COVERAGE) RAPORU"
	@echo "============================================================"
	gcov -r $(COV_TARGET)-test_runner.cpp
	@echo "============================================================"

clean:
	rm -f $(TARGET) $(LEGACY_TARGET) $(TEST_TARGET) $(COV_TARGET)
	rm -f *.gcda *.gcno *.gcov tests/*.gcda tests/*.gcno
	rm -rf output/main

.PHONY: all run run-malformed test coverage clean
