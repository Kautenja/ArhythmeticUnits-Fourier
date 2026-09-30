# Historical Benchmark Inputs

`configs/` preserves older expanded configurations, development investigations,
and the former confirmation plan. They are available for reproducing earlier
work, not the default configuration surface. Maintained profiles live in
[../profiles/](../profiles/README.md).

`prototype/` retains the original standalone experiment driver. Exact historical
reproduction should instead extract the source archive identified by the
[manuscript data](../../data/README.md), whose hashes and original paths are
unchanged. New comparison measurements use [../bench.py](../bench.py).

The archived optimization patches, measurements and one-off analysis programs
moved together to [../../data/research/](../../data/research/). They remain
research artifacts; they are not imported by the maintained runner. No prior
evidence was deleted or declared superseded by this directory cleanup.
