#!/bin/bash
QUAKE_EXIT_CODE=0
QUAKE_EXECUTABLE=$(which chocolate-quake)
QUAKE_PORT_NAME="Chocolate Quake"

if [[ ! -z "$FLATPAK_ID"]]; then
    QUAKE_COMMANDLINE="flatpak run $FLATPAK_ID"
else
    QUAKE_COMMANDLINE=$QUAKE_EXECUTABLE
fi

function check_game_data () {
  if [[ "$1" == "" ]]; then
    zenity --error --ok-label "Quit" --width=400 --text \
    "<b>Could not find Quake game data</b>\n\n Please either install Quake via Steam or copy the game data (at least <tt>pak0.pak</tt>) to <tt><b>$XDG_DATA_HOME/quake/id1/</b></tt>."
    exit 1
  fi
}

function check_exit_code () {
  if [[ "$1" != "0" ]]; then
    zenity --error --ok-label "Quit" --width=400 --text \
      "<b>$QUAKE_PORT_NAME exited with an error</b>\n\n For a detailed error message, please run $QUAKE_PORT_NAME from a terminal window using\n <tt><b>$QUAKE_COMMANDLINE</b></tt>."
    exit 1
  fi
}

echo "Checking ${XDG_DATA_HOME} for Quake game data..."
if [[ -f "$XDG_DATA_HOME/quake/id1/pak0.pak" ]]; then
  QUAKEDIR=$XDG_DATA_HOME/quake
  echo "Found Quake data in $QUAKEDIR!"
  cd $QUAKEDIR
  $QUAKE_EXECUTABLE "$@"
  check_exit_code $?

# Otherwise, check the Steam libraries
else
  echo "Checking Steam libraries for Quake game data..."
  LIBRARYFOLDERS_VDF_PATH=$(find ~ -ipath "*/config/libraryfolders.vdf")
  LIBRARY_PATHS=$(sed -nE "s:^\s+\"path\"\s+\"(.*)\"$:\1:p" $LIBRARYFOLDERS_VDF_PATH)
  for library in ${LIBRARY_PATHS[@]}; do
    echo "Checking in $library..."
    QUAKEDIR=$(find "$library" -ipath "*/steamapps/common/Quake/id1/pak0.pak" | sed s:/id1/pak0.pak::I)
    if [[ -d "$library" && "${QUAKEDIR}" != "" ]]; then
      echo "Found ${QUAKEDIR}!"
      cd $QUAKEDIR
      $QUAKE_EXECUTABLE "$@"
      check_exit_code $?
    fi
  done
fi

check_game_data $QUAKEDIR
