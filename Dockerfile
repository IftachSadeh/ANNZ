FROM kngenie/trusty-python2 

MAINTAINER Priscila Gutierres <priscila.gutierres@usp.br>

ENV PYTHONPATH=/software/ANNZ/examples

ENV  ROOTSYS=/software/root

ENV LD_LIBRARY_PATH=/software/root/lib

RUN apt-get update && apt-get install gcc wget libfreetype6 libssl-dev git -y

RUN cd /lib/x86_64-linux-gnu

RUN ln -s libssl.so.1.0.0 libssl.so.10

RUN ln -s libcrypto.so.1.0.0 libcrypto.so.10

RUN mkdir -p /software && cd /software

RUN wget  https://root.cern.ch/download/root_v5.34.11.Linux-slc6_amd64-gcc4.6.tar.gz

RUN tar -xzvf root_v5.34.11.Linux-slc6_amd64-gcc4.6.tar.gz

RUN git clone https://github.com/IftachSadeh/ANNZ.git

RUN cd /software/ANNZ

RUN python examples/scripts/annz_singleReg_quick.py --make

