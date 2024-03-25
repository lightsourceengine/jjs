// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// note: this tested a vm byte code issue, but src is tuned for 512K heap.
// if src is too big, null is thrown. it seems like the vm byte code issue
// was fixed, but there is an OOM condition issue with eval/parsing.

var src = "var a = 0; while(a) { switch(a) {";
/* The += operation has a longer opcode. */
for (var i = 0; i < 2270; i++)
    src += "case " + i + ": a += a += a; break; ";
src += "} }";

eval(src);
