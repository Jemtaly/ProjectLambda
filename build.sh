#!/bin/sh
make SOURCE=lambda_cbn.cpp
make SOURCE=lambda_cbn.cpp USE_GMP=1
make SOURCE=lambda_cbv.cpp
make SOURCE=lambda_cbv.cpp USE_GMP=1
