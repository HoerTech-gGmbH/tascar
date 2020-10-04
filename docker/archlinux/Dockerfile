FROM archlinux

RUN pacman --noconfirm -Syu git make gcc pkg-config libxml++2.6 jack2 liblo libsndfile fftw gsl eigen gtkmm3 boost libltc xxd gtksourceviewmm webkit2gtk

RUN useradd u
RUN mkdir /home/u
RUN chown u:u /home/u
RUN cd /home/u && git clone https://github.com/gisogrimm/tascar
RUN chown -R u:u /home/u/tascar

COPY entrypoint.sh /home/u/entrypoint.sh
COPY build.sh /home/u/build.sh

ENTRYPOINT ["/home/u/entrypoint.sh"]
