//===- NonRelocatableStringpool.h - A simple stringpool  --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_DSYMUTIL_NONRELOCATABLESTRINGPOOL_H
#define LLVM_TOOLS_DSYMUTIL_NONRELOCATABLESTRINGPOOL_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/DwarfStringPoolEntry.h"
#include "llvm/Support/Allocator.h"
#include <cstdint>
#include <utility>

namespace llvm {
namespace dsymutil {

/// A string table that doesn't need relocations.
///
/// We are doing a final link, no need for a string table that has relocation
/// entries for every reference to it. This class provides this ablitity by
/// just associating offsets with strings.
class NonRelocatableStringpool {
public:
  /// Entries are stored into the StringMap and simply linked together through
  /// the second element of this pair in order to keep track of insertion
  /// order.
  using MapTy = StringMap<DwarfStringPoolEntry, BumpPtrAllocator>;

  NonRelocatableStringpool() {
    // Legacy dsymutil puts an empty string at the start of the line
    // table.
    EmptyString = getEntry("");
  }

  DwarfStringPoolEntryRef getEntry(StringRef S);

  /// Get the offset of string \p S in the string table. This can insert a new
  /// element or return the offset of a pre-exisitng one.
  uint32_t getStringOffset(StringRef S);

  /// Get permanent storage for \p S (but do not necessarily emit \p S in the
  /// output section).
  /// \returns The StringRef that points to permanent storage to use
  /// in place of \p S.
  StringRef internString(StringRef S);

  uint64_t getSize() { return CurrentEndOffset; }

  std::vector<DwarfStringPoolEntryRef> getEntries() const;

private:
  MapTy Strings;
  uint32_t CurrentEndOffset = 0;
  unsigned NumEntries = 0;
  DwarfStringPoolEntryRef EmptyString;
};

} // end namespace dsymutil
} // end namespace llvm

#endif // LLVM_TOOLS_DSYMUTIL_NONRELOCATABLESTRINGPOOL_H
