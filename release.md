# Create a new release

On branch development:

- build, test and package: `make clean; make pack`

- increment version number in `config.mk`, edit `changelog`

- commit changes to git (do not push)

- create release tag: `make releasetag`

- swich to master branch, merge development

- push tags and master to all remotes

- switch back to development branch


