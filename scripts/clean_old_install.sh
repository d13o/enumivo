#! /bin/bash

if [ -d "/usr/local/include/enumivo" ]; then
   printf "\n\tOld enumivo install needs to be removed.\n\n"
   printf "\tDo you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\n\tThis requires sudo, please run ./scripts/clean_old_install.sh with sudo\n\n"
               exit -1
            fi
            pushd /usr/local &> /dev/null
            pushd include &> /dev/null
            rm -rf appbase chainbase enumivo enu.system enulib fc libc++ musl &> /dev/null
            popd &> /dev/null

            pushd bin &> /dev/null
            rm enucli enugenabi enuapplesdemo enulauncher enumivo-s2wasm enumivo-wast2wasm enumivocpp enuwallet enunode &> /dev/null
            popd &> /dev/null

            pushd etc &> /dev/null
            rm enumivo &> /dev/null
            popd &> /dev/null

            pushd share &> /dev/null
            rm enumivo &> /dev/null
            popd &> /dev/null

            pushd usr/share &> /dev/null
            rm enumivo &> /dev/null
            popd &> /dev/null

            pushd var/lib &> /dev/null
            rm enumivo &> /dev/null
            popd &> /dev/null

            pushd var/log &> /dev/null
            rm enumivo &> /dev/null
            popd &> /dev/null
            break;;
         [Nn]* ) 
            printf "\tAborting uninstall\n\n"
            exit -1;;
      esac
   done
fi
