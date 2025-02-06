import Mathlib.Data.Multiset.Basic


inductive Op : Type :=
  | op_add : Op
  | op_sub : Op
  | op_cnt : Nat -> Op


structure history where
