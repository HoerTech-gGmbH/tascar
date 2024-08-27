# do not package main until hoa2d tests are fixed!

# Create a new release

On branch development:

- build, test and package: `make clean; make -j 8 && make pack`

- increment version number in `config.mk`, edit `changelog`

- edit doc/news/index.html

- commit changes to git (do not push)

- create release tag: `make releasetag`

- swich to master branch, merge development

- push tags and master to all remotes

- switch back to development branch

- create release on github, both repos

- create a brew release

- update release on zenodo
