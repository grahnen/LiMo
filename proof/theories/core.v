From HB Require Import structures.
From Coq Require Import ssreflect ssrbool ssrfun.
From mathcomp Require Import ssrnat eqtype fintype seq path bigop interval finset choice.
From pcm Require Import options axioms pred prelude rtc seqext useqord uslice uconsec.
From pcm Require Import pcm automap heap natmap morphism finmap.



Module ExistT2Notation.

  Notation "( x ; y ; z )" := (@existT2 _ _ _ x y z) (at level 0, format "( x ;  '/  ' y ; '/ ' z )").
  Notation "( x ; y ; z ) :> A P Q" := (@existT2 A P Q x y z) (at level 0, format "( x ;  '/  ' y ; '/ ' z ) :> A P Q").
End ExistT2Notation.

Import ExistT2Notation.

Section SubsetMap.
  Variables T A : eqType.
  Implicit Types (s t : seq T).
  Implicit Types f : T -> option A.

  Lemma sub_undup s t : {subset s <= t} -> {subset (undup s) <= (undup t)}.
  Proof.
    move => sub_s x.
    rewrite !mem_undup.
    by apply sub_s.
  Qed.

  Lemma sub_pmap s t f : {subset s <= t} -> {subset pmap f s <= pmap f t}.
  Proof.
    move => sub_s.
    move => x.
    rewrite !mem_pmap.
    by apply sub_map.
  Qed.

End SubsetMap.

Section TagTagEq.

  Variables (I : eqType) (T1 T2 : I -> eqType).
  Implicit Types u v : { i : I & T1 i & T2 i }.

  Definition tag2_eq u v : bool := (tag u == tag v) && (tagged u == tagged_as u v).

  Lemma tag2_eqP : Equality.axiom tag2_eq.
  Proof.
    rewrite /tag2_eq => [] [i i2 i3] [j] /=.
    case: eqP => [<-|Hij] j2 j3; last by right; case.
    apply: (iffP eqP); rewrite tagged_asE //=.
    - move => /inj_pair [eq2 eq3];
      apply (eq_existT2_curried (erefl i)); rewrite //=.
    - case=> //=; move => [/inj_pair2 eq2] [/inj_pair2 eq3]. by apply pair_equal_spec.
  Qed.

  Check fun t a b =>  ((t ; a ; b) : {t : I & T1 t & T2 t}).

  Lemma tag2_eq_tag (t : I) : forall (a1 a2 : T1 t) (b1 b2 : T2 t), (((t ; a1 ; b1 ) : { x : I & T1 x & T2 x}) = (t ; a2 ; b2)) -> a1 = a2 /\ b1 = b2.
  Proof. move => a1 a2 b1 b2 eq. inversion eq.
         move /inj_pair2 in H0. move /inj_pair2 in H1.
         by [].
  Qed.

  HB.instance Definition _ := hasDecEq.Build ({i : I & T1 i & T2 i }) tag2_eqP.
End TagTagEq.


Inductive EvTp :=
| Add
| Remove
| Empty.

Definition evTp_eq (a b : EvTp) : bool :=
  match (a, b) with
  | (Add, Add) => true
  | (Remove, Remove) => true
  | (Empty, Empty) => true
  | (_, _) => false
  end.

Lemma eq_evTp : eq_axiom evTp_eq.
Proof.
  case; case; constructor => //=.
Qed.

HB.instance Definition _ := hasDecEq.Build EvTp eq_evTp.


Definition EvTp_seq := [:: Add; Remove; Empty].

Lemma EvTp_fin : Finite.axiom EvTp_seq.
Proof. by case. Qed.

HB.instance Definition _ := isFinite.Build EvTp EvTp_fin.
HB.about EvTp.

Check (EvTp : finType).



Definition q : {set EvTp}.

Definition argTp (T : ordType) (e : EvTp) : ordType :=
  match e with
  | Empty => unit
  | _ => T
  end.

Notation Event A := { e : EvTp & argTp A e }.

Notation "! a" := (existT (@argTp _) Add a) (at level 0).
Notation "? a" := (existT (@argTp _) Remove a) (at level 0).
Notation "?-" := (existT (@argTp _) Empty tt) (at level 0).

Section DomLemmas.
  Variables (K : ordType) (C : pred K) (V : Type) (U : union_map C V).
  Implicit Types (k : K) (v : V) (f g : U).

  Lemma find_dom_some k f : k \in dom f -> exists v, find k f = Some v.
  Proof. case: dom_find => v //= H _ _. by apply (ex_intro _ v). Qed.
End DomLemmas.

Section Mapping.
  Variables (n0 : nat) (T1 : eqType) (x1 : T1).
  Variables (T2 : eqType) (x2 : T2) (f : T1 -> T2).
  Implicit Type s : seq T1.

  Lemma nth_map_index s default x :
    x \in s -> nth  default (map f s) (index x s) = f x.
  Proof.
    (* elim: s => //= y s IHs inj_f s_x.  *)
    elim: s => //= y s IHs s_x; rewrite ?mem_head //.
    move: s_x; rewrite inE; have [-> // | _] := eqVneq; apply: IHs.
  Qed.

End Mapping.
