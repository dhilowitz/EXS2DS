# EXS2DS
A command-line utility that converts Logic Sample (EXS) files to Decent Sampler format. At this point, it handles only the most basic mappings, but it's a start.

## Usage

```
./EXS2DS <exs-file> <ds-preset-file> [sample-directory]
```

Only use the sample directory if you want the utility to overwrite any existing sample paths so that it looks in a specific sample directory.

## Example Usage

```
./EXS2DS Test.exs Test.dspreset "Path/To/Samples/"
```
