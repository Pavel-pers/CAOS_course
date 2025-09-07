FROM domwst/caos-private-ci

WORKDIR /public

RUN update-alternatives --install /usr/bin/cc cc $(which gcc) 30
RUN update-alternatives --install /usr/bin/c++ c++ $(which g++) 30

ADD --keep-git-dir https://gitlab.com/oleg-shatov/hse-caos-public.git .

RUN ./common/tools/build_all_tasks.sh
