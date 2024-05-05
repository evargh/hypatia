FROM ubuntu:22.04 as build

RUN apt-get update && apt-get install -y libproj-dev proj-data proj-bin libgeos-dev git openmpi-bin openmpi-common openmpi-doc libopenmpi-dev lcov gnuplot unzip
RUN apt-get install -y python3 python3-pip

ENV PIP_ROOT_USER_ACTION=ignore
RUN python3 -m pip install numpy astropy ephem networkx sgp4 geopy matplotlib statsmodels git+https://github.com/snkas/exputilpy.git@v1.6 cartopy git+https://github.com/snkas/networkload.git@v1.3

# git submodule update --init --recursive
# git config --global -add safe.directory '*'
