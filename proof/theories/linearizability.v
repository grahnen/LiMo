From HB Require Import structures.
From Coq Require Import ssreflect ssrbool ssrfun.
From mathcomp Require Import ssrnat eqtype fintype seq path bigop interval finset.
From pcm Require Import options axioms pred prelude rtc seqext useqord uslice uconsec.
From pcm Require Import pcm automap heap natmap morphism.
From LiMo Require Import core.

Section Operation.
  Variable A : ordType.
  Definition OpSet := fset (Event A).

  Definition history A := seq (Operation A).


  Fixpoint sequentialization (h : history A) : bool :=
    match h with
    | (x :: y :: xs) => not (lt y x)
    | _ => true
    end.



  Record Linearization (h : ) := {

    }.

End Operation.

Section History.
