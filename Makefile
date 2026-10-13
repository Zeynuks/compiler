# Юзать `make BUILD_TYPE=Release` для смены типа сборки
BUILD_TYPE = Debug

all: cmake build test

cmake:
	cmake -S . -B build/$(BUILD_TYPE) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build:
	cmake --build build/$(BUILD_TYPE) --parallel

test: build
	LSAN_OPTIONS="suppressions=${PWD}/tests/leak-sanitizer-suppressions.txt" \
	ctest --output-on-failure --test-dir build/$(BUILD_TYPE)

# Сборка с инструментированием, запуск тестов и HTML-отчёт о покрытии в coverage/index.html.
# Требует gcovr: `pip install gcovr`.
coverage:
	cmake -S . -B build/Coverage -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
	cmake --build build/Coverage --parallel
	find build/Coverage -name "*.gcda" -delete
	ctest --output-on-failure --test-dir build/Coverage
	mkdir -p coverage
	gcovr --root . --filter src/ --object-directory build/Coverage \
		--html-details coverage/index.html --print-summary

format:
	find src/ tests/ -name "*.h" -o -name "*.cpp" | xargs clang-format -i

clean:
	rm -rf build/ coverage/

.PHONY: all cmake build test coverage format clean
