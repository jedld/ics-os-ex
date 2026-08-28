/* ICS-OS single-target BFD vector table.
   Replaces the configure-generated targmatch.h. Only the ELF64 x86-64
   vectors (plus the l1om/k1om ABIs that share elf64-x86-64.c) are selected.
   Included inside `static const struct targmatch bfd_target_match[] = { ... }`
   in bfd/targets.c, so this file must contain only initializer entries.
   Each vector field is a `const bfd_target *`, so the entries take the
   address of the (extern const) vector symbol, matching the real sed output. */
{ "x86_64", &bfd_elf64_x86_64_vec },
{ "x86_64-elf", &bfd_elf64_x86_64_vec },
{ "i386", &bfd_elf64_x86_64_vec },
{ "amd64", &bfd_elf64_x86_64_vec },
{ "x86-64", &bfd_elf64_x86_64_vec },
{ "i386-l1om", &bfd_elf64_l1om_vec },
{ "i386-k1om", &bfd_elf64_k1om_vec },
