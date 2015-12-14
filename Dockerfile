FROM gcc:4.8
RUN mkdir -p /usr/chenyg/apiserver
WORKDIR /usr/chenyg/apiserver
COPY . /usr/chenyg/apiserver
RUN make clean&&make
EXPOSE 8088
RUN chmod u+x /usr/chenyg/apiserver/start.sh
CMD /usr/chenyg/apiserver/start.sh

