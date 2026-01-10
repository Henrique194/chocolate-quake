include(CheckSymbolExists)

# Check if the symbol exists in the specified header file,
# and if it does, add variable to compile definitions.
macro(check_define sym header def_var)
  check_symbol_exists(${sym} ${header} ${def_var})
  if(${def_var})
    add_compile_definitions(${def_var})
  endif()
endmacro()
