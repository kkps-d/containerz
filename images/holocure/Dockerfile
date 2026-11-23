FROM containerz/streamer:dev

USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends wine wine64 fonts-wine \
    && rm -rf /var/lib/apt/lists/* \
    && install -d -m 0700 -o player -g player /home/player/.wine-holocure

ENV WINEPREFIX=/home/player/.wine-holocure \
    WINEDLLOVERRIDES="mscoree,mshtml=" \
    WINEDEBUG=fixme-all \
    CONTAINERZ_STREAM_CURSOR=game-drawn

COPY --chmod=0755 start-holocure.sh /usr/local/bin/start-holocure
CMD ["start-holocure"]
