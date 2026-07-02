package fuzztemplate

import (
    "Mugen/pkg/profile/yaotl"
    "Mugen/pkg/profile/yaotl/hclsyntax"
)

func Fuzz(data []byte) int {
    _, diags := hclsyntax.ParseTemplate(data, "<fuzz-tmpl>", hcl.Pos{Line: 1, Column: 1})

    if diags.HasErrors() {
        return 0
    }

    return 1
}
