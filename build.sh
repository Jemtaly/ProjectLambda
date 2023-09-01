#!/bin/sh
make SOURCE=lambda_lite.cpp
make SOURCE=lambda_lite.cpp USE_GMP=1
make SOURCE=lambda_full.cpp
make SOURCE=lambda_full.cpp USE_GMP=1
