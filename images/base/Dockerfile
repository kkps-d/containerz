# containerz/base: shared foundation for all images.
FROM ubuntu:26.04

ARG USERNAME=player
ARG UID=1000
ARG GID=1000

ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    TZ=Etc/UTC \
    DEBIAN_FRONTEND=noninteractive \
    XDG_RUNTIME_DIR=/run/user/${UID} \
    ENABLE_SSH=1

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates tzdata tini \
        sudo nano less bash-completion \
        curl wget iproute2 iputils-ping dnsutils netcat-openbsd \
        openssh-server \
        procps htop psmisc lsof strace \
        file tree unzip xz-utils zstd \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu images ship an "ubuntu" user on UID 1000; replace it with ours.
RUN if id ubuntu >/dev/null 2>&1; then userdel -r ubuntu; fi \
    && groupadd -g ${GID} ${USERNAME} \
    && useradd -m -s /bin/bash -u ${UID} -g ${GID} ${USERNAME} \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME} \
    && install -d -m 0700 -o ${UID} -g ${GID} ${XDG_RUNTIME_DIR} \
    && install -d -m 0700 -o ${UID} -g ${GID} /home/${USERNAME}/.ssh

# SSH: key-only login, no root login.
RUN printf '%s\n' \
        'PasswordAuthentication no' \
        'KbdInteractiveAuthentication no' \
        'PermitRootLogin no' \
        > /etc/ssh/sshd_config.d/10-containerz.conf \
    && rm -f /etc/ssh/ssh_host_*

COPY --chmod=0755 entrypoint.sh /usr/local/bin/entrypoint.sh

ENV CONTAINERZ_USER=${USERNAME}
EXPOSE 22
WORKDIR /home/${USERNAME}

# Entrypoint starts as root (for sshd), then drops to the non-root user.
ENTRYPOINT ["/usr/bin/tini", "--", "/usr/local/bin/entrypoint.sh"]
CMD ["bash"]
