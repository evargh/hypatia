## Steps

1. Clone the repository to a folder `hypatia/` on your machine.
2. In `hypatia/ns3-sat-sim/`, edit `build.sh` and add `--disable-werror` to each `./waf configure <arguments>`. This is because modern versions of g++ produce a warning for a program in the submodule, which stops the build.
3. Move the Dockerfile into a separate directory from the repository. Run `docker build -t ns3 -f Dockerfile` in that directory.
4. When this finishes, run `docker run -it -v <path/to/local/hypatia>:/media/hypatia ns3:latest`.
5. Now, in the container, go to `/media/hypatia`. Run `git submodule update --init --recursive`. Depending on how you configured docker, you may have to run `git config --global --add safe.directory="*"` as well. For user-level podman I didn't have to, but for running docker as root I did.
6. Execute `hypatia_build.sh,` and when that's done, execute `hypatia_run_tests.sh`. 
7. Done. Since we mounted the directory, any changes you make will be reflected in the actual directory, but any object files will be built for Ubuntu.

