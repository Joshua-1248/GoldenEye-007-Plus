FROM ubuntu:22.04

RUN echo ">> install distro packages ..." &&  \
    apt update && \
    apt -y install bash-completion sudo binutils-mips-linux-gnu wget make git python3 build-essential

# Hint: comment out qemu installation, if you're using ido recomp
RUN echo ">> install qemu ..." &&  \
    wget https://github.com/n64decomp/qemu-irix/releases/download/v2.11-deb/qemu-irix-2.11.0-2169-g32ab296eef_amd64.deb -P /tmp && \
    dpkg -i /tmp/qemu-irix-2.11.0-2169-g32ab296eef_amd64.deb

RUN echo ">> setup workspace ..." &&  \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers && \
    useradd -ms /bin/bash dev && usermod -aG sudo dev
USER dev
WORKDIR /home/dev/project

CMD ["/bin/bash"]
