# ExamLens-Syllabus-PYQ-Analysis-System
ExamLens is a C++-based DSA and OOP project that analyzes subject syllabus and previous-year question papers to identify important topics, repeated questions, frequency trends, and study priorities. Uses Hash Maps, Trie, Priority Queue, Sorting, and String Matching algorithms for data-driven exam preparation.
# Architecture
GUI → Application Layer (OOP services) → DSA Core → Data Layer (parsers + storage)
Flow: Import Syllabus → Import PYQs → Validate → Analyze → Dashboard → Report
# Changes After Phase I
Interface: CLI output → interactive GUI dashboard
Repetition: identical/similar wording → 4 classes (Exact, Normalized, Similar Pattern, Unique)
Pattern analysis: questions grouped by underlying task (e.g. *BST Construction*), not just wording
- Priority: frequency-based ranking → explainable 0–100 score with reasons for every topic
- Trends: basic frequency → year-wise charts with labels (Increasing, Decreasing, Stable, Fluctuating, Occasional, Recently Active)
- Marks: new marks distribution per topic and per unit
- Unit analysis: new unit-wise questions, marks, coverage and priority view
- Study plan:ranked list → prerequisite-aware study order (Graph + Topological Sort)
- Search: basic lookup → Trie-based search with autocomplete and filters (year, unit, marks, type)
- Data checks: new Data Quality Center (unmapped questions, missing marks, duplicates, manual review)
- Practice: new Practice Mode (most repeated, high-priority, high-mark, topic-wise, year-wise, random)
- Output: console print → one-page final student report
  
