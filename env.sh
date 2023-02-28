if ! echo "$PATH" | grep -q 'gcc-arm' ; then
  export PATH="$PATH:$(pwd)/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin"
fi
