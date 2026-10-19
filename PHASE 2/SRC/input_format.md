# ExamLens input format (Data/Input layer)

Three plain-text files, pipe-separated, `#` starts a comment. UTF-8.

## questions.txt
```
YEAR: 2024
Q1a   | 5  | Define a data structure. Differentiate linear and non-linear structures.
Q3    | 5  | Construct a BST for 50, 30, 70 and give its inorder traversal.
Q3-or | 5  | Construct an AVL tree for 10, 20, 30 showing rotations.
Q4    |    | Write the merge sort algorithm.          <- blank marks = missing
```
- `YEAR:` once above each paper; every question below belongs to it.
- `QID` is `Q` + number + optional sub-part letter + optional `-or` for the OR alternative.
- Blank marks field means the paper did not print marks. It is kept but excluded from marks totals.
- The last field keeps the rest of the line, so question text may contain `|`.

## syllabus.txt
```
ALIAS: bst = binary search tree          one word = its full form
UNIT: 4
TOPIC: AVL trees | avl, avl tree, rotation, balance factor
```
- Aliases are applied to questions and keywords before matching.
- The topic name is itself a keyword.

## prereqs.txt
```
Trees and traversals -> Binary search trees     study A before B
```

## Validation rules
| Check | Rule |
|---|---|
| missing year | question with no `YEAR:` above it |
| missing marks | blank or invalid marks field |
| duplicate | same normalized text twice in one year (data-entry error) |
| duplicate id | same QID twice in one year |
| orphan OR | `-or` row with no base question |

The same question across different years is not an error. That is repetition, handled downstream.
