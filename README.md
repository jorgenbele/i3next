# i3next
A simple utility for switching to another [i3](https://github.com/i3/i3) workspace, written in c using [i3ipc-glib](https://github.com/acrisci/i3ipc-glib).

## Features
* Switch to the previous/next non-empty and empty workspace.
* Switch to an empty workspace.

## Limitations
* Only supports *i3*, not *sway* or other *i3-ipc* compatible window managers. This is caused by its relience on i3ipc-glib, which seems to only support *i3*.
* Workspaces must have numerical workspace-names.

## Installation
```
git clone https://github.com/jorgenbele/i3next
cd i3next
make
make install # installs to $HOME/bin
```

## Usage
```
Usage: ./i3n [-efhnp] 
  -e	Find the nearest empty workspace and select it
  -f	Force change to next workspace (even if none is active)
  -h	Shows this message and quits
  -n	Try to change to the next workspace
  -p	Try to change to the previous workspace
```
