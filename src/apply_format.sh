#!/bin/bash

find . -type f -name "*.hpp" -exec clang-format -i {} +
find . -type f -name "*.cpp" -exec clang-format -i {} +
