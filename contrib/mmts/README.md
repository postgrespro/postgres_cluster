# Postgres Multimaster

## Testing

The testing process involves multiple modules that perform different tasks. The
modules and their APIs are listed below.

### Modules

#### `combineaux`

Governs the whole testing process. Runs different workloads during different
troubles.

#### `stresseaux`

Puts workloads against the database. Writes logs that are later used by
`valideaux`.

* `start(id, workload, cluster)` - starts a `workload` against the `cluster`
and call it `id`.
* `stop(id)` - stops a previously started workload called `id`.

#### `starteaux`

Manages the database nodes.

* `deploy(driver, ...)` - deploys a cluster using the specified `driver` and
other parameters specific to that driver. Returns a `cluster` instance that is
used in other methods.
* `cluster->up(id)` - adds a node named `id` to the `cluster`.
* `cluster->down(id)` - removes a node named `id` from the `cluster`.
* `cluster->drop(src, dst, ratio)` - drop `ratio` packets flowing from node
`src` to node `dst`.
* `cluster->delay(src, dst, msec)` - delay packets flowing from node `src` to
node `dst` by `msec` milliseconds.

#### `troubleaux`

This is the troublemaker that messes with the network, nodes and time.

* `cause(cluster, trouble, ...)` - causes the specified `trouble` in the
specified `cluster` with some trouble-specific parameters.
* `fix(cluster)` - fixes all troubles caused in the `cluster`.

#### `valideaux`

Validates the logs of stresseaux.

#### `reporteaux`

Generates reports on the test results. This is usually a table that with
`trouble` vs `workload` axes.
