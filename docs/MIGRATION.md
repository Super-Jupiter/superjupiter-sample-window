# Version policy

0.1.0 is the first public preview. There is no previous public API to migrate.
Use `superjupiter::sample_window` and the branded public header/package name.
All new SuperJupiter components use their own namespace. No root aliases are added.

Future breaking 0.x changes will increase the minor version and include explicit
migration notes. Patches preserve the documented API. Published tags/packages
will not be overwritten. The compatible CMake range is the same minor version.
Use exact pins when reproducibility is required. Hardware validation is pending.


## 0.1.1 compatibility review

Package, header, component namespace and CMake target names are unchanged. Existing tags and storage byte formats are preserved. Read statuses and numeric-validity flags before using derived values; handle additional diagnostic enum values in switches. TimeAggregate adds Summary.numericError; RecordJournal/ConfigStore add StaleState. Numeric-invalid observations retain chronology in ThresholdEvents, MachineState and Integration. Reopen stale storage handles under external writer exclusion. See INTEROPERABILITY.md and the release changelog for component-specific fixes. Hardware validation pending.
