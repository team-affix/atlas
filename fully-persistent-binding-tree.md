# Fully Persistent Binding Environments: A Conversation

*A discussion of data structures for tree-shaped binding environments with multiple simultaneously-active branches.*

---

## Question 1

> is there a data structure that supports the following efficiently in time+space:
>
> something like prolog's backtrackable bind map (unification union find structure) but where instead, we support MULTIPLE active paths of bindings at the same time? like binding exploration but where we don't need to "backtrack" one path to get to another branch of the tree,
>
> so e.g.
>
> imagine a tree where each node in the tree introduces more bindings upon the environment given by the ancestors above that node. Each branch is independent but multiple branches can share an ancestor path which means they share a partial binding environment.
>
> what I want to achieve though is still something like either log(n) or amortized O(1) lookup of a binding from any node in the tree, like we have with union find data structures, but here's the catch: I want the memory usage to be O(n) at depth n. So in other words, no "bind map at every node" since each bind map would have O(n) vars, and n nodes having n vars means quadratic.

### Answer

Yes. What you're describing is exactly a **fully persistent array**, and the classic construction is Dietz's (1989), which hits your constraints: O(1) space per binding (so O(n) total for n bindings, regardless of how many nodes see them), O(1) amortized update, O(log log n) lookup from any node.

#### The reframing

"Lookup var x at node v" = "find the deepest ancestor of v that bound x." Per-variable, that's a *nearest marked ancestor* query. Per-variable-list, it's a *stabbing query on nested intervals*, which reduces to predecessor search.

Concretely:

**1. Euler-tour labels via order maintenance.** When a node is created, allocate two labels `open(v) < close(v)` from an order-maintenance structure (Dietz–Sleator list labeling: O(1) amortized insert, O(1) compare). A child's pair gets inserted strictly between its parent's pair. Then `u` is an ancestor of `v` iff `open(u) < open(v) < close(u)`. This works online while the tree is still growing and while many branches are simultaneously open, which is the part a static DFS numbering can't do.

**2. Per-variable event list.** For each variable x, keep a predecessor structure over labels. When node u binds x to value `t`, insert two events:

- at `open(u)`: value `t`
- at `close(u)`: value = whatever x resolved to at u's parent (the shadowed binding)

**3. Lookup(v, x)** = predecessor of `open(v)` in x's list, return its recorded value. The open/close pairing is what makes the naive version correct: without close events, the predecessor by position might be a node in a sibling subtree that already finished, and you'd have to walk backwards an unbounded number of steps. With them, the single predecessor query is always the right answer.

Space is 2 words per binding, so a root-to-leaf path of depth n contributes O(n) and never n². Nodes that bind nothing cost O(1). Sharing is automatic because ancestors' events are simply still in the list.

Lookup cost depends on what you use for step 3:

- sorted dynamic array or balanced BST: O(log k_x) where k_x is how many times *that variable* was bound anywhere in the tree, usually tiny
- y-fast trie / vEB over the integer labels: O(log log n), which is Dietz's stated bound

#### Adding unification on top

You want union-find, not just a map. The standard move is Conchon and Filliâtre's *A Persistent Union-Find Data Structure* (2007): represent the parent array and rank array as persistent arrays and let `find` do path compression by *writing* to the persistent array, which just appends events in your current branch. Compression stays local to the path that performed it, so branches don't corrupt each other. The amortization argument gets murky under persistence, but empirically it behaves.

One caveat: their paper's persistent arrays use the *rerooting* trick, which is fast only when access is single-threaded and backtracking-shaped. Rerooting under two genuinely interleaved active paths thrashes, with each switch costing the diff distance. That's precisely the case you said you want to avoid, so swap their array implementation for Dietz-style and keep the union-find layer.

#### The bad news

Gupta and Jayaraman proved (NACLP '90, later TOPLAS) that for OR-parallel logic programming, no execution model can simultaneously achieve constant-time environment creation, constant-time binding access, and constant-time task switching. Your question is that problem verbatim: a tree of branches sharing ancestor binding environments. So O(1) worst-case on all three is out, and O(log log n) access with O(1) creation and O(1) switching is near the ceiling.

The whole design space of that literature is worth skimming, since each point picks two of three: Warren's SRI **binding arrays** (O(1) access, O(depth) switch), Ciepielewski–Haridi **hashing windows**, **version vectors**, **time-stamping**, and Muse's **stack copying**.

#### Cheaper things that may actually be what you want

**If you can control the schedule.** If you're willing to process the tree in DFS order and only ever query at the node you're currently sitting on, use a plain mutable array plus a trail: push old value on bind, pop on exit. O(1) lookup, O(1) bind, O(n) space, no cleverness. Multiple branches cost nothing as long as you finish one subtree before entering the next. The entire fancy machinery above is only needed when you must query arbitrary nodes at arbitrary times, e.g. a work-stealing scheduler or a demand-driven evaluator that hops around.

**If you want something you can write this afternoon.** A persistent HAMT keyed by variable: O(log₃₂ n) lookup, structure sharing for free, path copying of ~2–3 nodes per bind. Space is technically O(n log n) words but the constant makes it behave like small-constant O(n) at realistic sizes. This is what most functional logic engines and Clojure/Scala-style implementations actually do, and it's within a small factor of Dietz in practice while being an order of magnitude less code.

**If the tree is fully built before you query.** Skip order maintenance entirely: one DFS to assign integer `tin`/`tout`, per-variable sorted arrays, binary search. Or go fully offline: single DFS maintaining a per-variable stack, answering all queries at each node in O(1) as you pass through.

My suggestion: start with the HAMT, and only move to Euler labels plus per-variable predecessor lists if profiling shows the map lookups dominating.

---

## Question 2

> Space is 2 words per binding, so a root-to-leaf path of depth n contributes O(n)
>
> hmm. but where does this data sit? does it sit at only one node? or is it computed based on traversing from root to that node? or is it moreso just like immediately obvious based on the tree structure given a node position?

### Answer

None of the three, quite — and the reason it's confusing is that the storage is *transposed* relative to how you're picturing it.

#### The data doesn't live at nodes at all

A node stores O(1) words, permanently:

```
Node { open: Label, close: Label, parent: NodeId }
```

No map, no bindings, nothing that grows with depth. The actual binding data lives in a **global, variable-indexed side table**:

```
VarTable: VarId -> PredecessorStruct<Label, Value>
```

One dynamic predecessor structure per *variable*, shared by the entire tree. When node `u` binds `x`, it doesn't write anything into itself — it inserts two entries into `VarTable[x]`, keyed by `u`'s labels. The node authored the binding; the binding is filed under the variable.

So lookup is not a traversal (that would be O(depth)), and it's not readable off the tree pointers alone. It's a single predecessor query in a global index, where the node's label acts as the search key.

#### Worked example

```
root  binds x=1     open=0   close=100
├── A binds x=2     open=10  close=40
│   └── B binds y=7 open=20  close=30
└── C binds y=9     open=50  close=60
```

The two side tables, in full — this is the entire state of the system:

```
VarTable[x] = { 0: 1,  10: 2,  40: 1,  100: unbound }
VarTable[y] = { 20: 7, 30: unbound, 50: 9, 60: unbound }
```

`lookup(v, x)` = predecessor-or-equal of `open(v)` in `VarTable[x]`:

- `lookup(B, x)`: pred(20) → key 10 → **2**. B inherits A's binding. Note B stored nothing about x.
- `lookup(C, x)`: pred(50) → keys 0, 10, 40 are ≤ 50, take 40 → **1**. C correctly sees root's x, not A's. This is what the close event buys you — without the entry at 40, the predecessor would be A's binding at 10, from a sibling subtree C never entered.
- `lookup(A, y)`: pred(10) → nothing ≤ 10 → **unbound**. Correct, y is bound only below and beside A.
- `lookup(C, y)`: pred(50) → key 50 → **9**. A node's own binding sits exactly at its open label, so predecessor-or-equal picks it up.

#### Why this is O(n) and not O(n²)

Inheritance is represented by the *absence* of a nearer event, not by a copy. B inheriting `x=2` costs zero words. A path of depth n with one bind per node produces exactly 2n entries total across all the tables. Nodes that bind nothing cost only their own two labels.

Compare with per-node maps: there, B would physically contain `{x: 2, y: 7}`, and every descendant re-materializes the whole environment. That's the quadratic blowup you were trying to dodge.

#### Two implementation details this exposes

**The close-event value requires a lookup.** When `u` binds `x`, you need to know what `x` resolved to at `u`'s parent, to write at `close(u)`. That's one predecessor query, so a bind costs a lookup plus two inserts.

**The per-variable structures are not append-only.** With several branches live at once, you might bind `x` deep in one branch, then later bind `x` in a branch whose labels sort *earlier*. So you need real dynamic inserts into the middle: a balanced BST gives O(log k) where k is how often that one variable was bound anywhere, and a y-fast trie over the integer labels gives O(log log n). A sorted vector only works if you build the tree in strict left-to-right DFS order, at which point the trail-and-undo approach is simpler anyway.

The thing that makes the labels usable while the tree is still growing is the order-maintenance structure — it hands you a label strictly between two existing ones in O(1) amortized, so you never have to renumber when a new child appears between existing subtrees.

---

## Question 3

> how many var entries for depth n? is it n^2?

### Answer

No — it's exactly **2 per bind operation**, and nothing about it scales with depth.

The count is:

```
total entries = 2 × (number of bind ops anywhere in the tree)
```

For a root-to-leaf path of depth n with one bind per node: n binds → **2n entries**. Linear.

Checking against the earlier example: 4 binds (root:x, A:x, B:y, C:y) → 8 entries, and the two tables held 4 each. ✓

#### Where the n² intuition comes from

Per-node maps make *inheritance* cost storage. A node at depth d materializes all d bindings visible to it, so the total is 1+2+3+...+n ≈ n²/2.

The side-table version never materializes inheritance. B seeing `x=2` is not a stored fact — it's the *outcome of a query* over entries that A already paid for. The only writes are at the moments a binding actually changes. n changes, 2n entries, done.

#### The distribution across tables

`VarTable[x]` holds 2 × (number of nodes that bind x). So the tables are individually small unless one variable is rebound a lot, and they sum to 2B overall. That also means the lookup cost with a balanced BST is O(log k_x) in the count for *that* variable, not O(log n) in the tree size — usually a handful of entries.

#### Two things that do scale differently

**Multiple binds per node.** A node binding 5 vars contributes 10 entries. The bound is in binds, not nodes, so it's O(B) generally; depth n is just the special case where B = n.

**Whole tree, not one path.** If the full tree performs B binds across all branches, you store 2B. That's linear in total work performed, which is the best you could hope for — you can't represent B distinct bindings in less than B space. The win is that branches sharing an ancestor share its entries for free rather than each re-copying them.

---

## Question 4

> okay so .... wow...
>
> so please explain this to me like I haven't heard of that data structure before

### Answer

#### Start with the problem

You have a tree. Each node says something like "x is 5" or "y is 7". A node inherits everything its ancestors said, unless it overrides it.

The obvious implementation gives each node a dictionary holding everything visible there. That's fast to read and hopeless on space, since a node at depth 500 stores 500 entries and so does its sibling, and so does everyone.

The fix comes from asking a different question. Instead of "what does node B know," ask:

> Among B and its ancestors, **who most recently touched x?**

Whoever that is, their value is the answer. Every other binding of x in the tree is irrelevant. So you don't need a dictionary at B. You need a way to find one specific node quickly.

#### Turning the tree into a line

Here's the trick that makes it work. Imagine walking the tree, the way you'd trace around a family tree with a pencil without lifting it: you enter a node, walk down through all its children, come back up, leave. Write down the clock time when you **enter** each node and when you **leave** it.

```
root      enter 0 ................................ leave 100
├── A     enter 10 ........... leave 40
│   └── B enter 20 .. leave 30
└── C     enter 50 .. leave 60
```

Now something nice has happened. **Ancestry became arithmetic.** A is an ancestor of B because the walk entered A at 10, and hadn't left yet at 20 when it entered B. B's entry time sits inside A's interval. C's entry time, 50, does not sit inside A's interval of 10 to 40, so A is not C's ancestor.

Ancestry checks are now two integer comparisons. No pointer walking.

#### Filing the bindings

Now flip where the data lives. Instead of storing bindings at nodes, keep **one timeline per variable**, shared by the whole tree. On the timeline for x, mark the moments x changed:

```
timeline for x:   time 0 → 1        (root said x=1)
                  time 10 → 2       (A said x=2)

timeline for y:   time 20 → 7       (B said y=7)
                  time 50 → 9       (C said y=9)
```

Each node itself stores nothing but its two times and a parent pointer. Three fields, forever, no matter how deep.

#### Looking things up

To ask "what is x at node B," take B's entry time, 20, and look **backwards along x's timeline** for the most recent mark at or before 20.

That's time 10, value 2. B inherits A's binding, and B stored nothing to make that true.

Looking backwards on a sorted list of numbers is binary search, so this is fast. Even better, each timeline only has marks for the times *that one variable* changed, so it's usually a very short list.

#### The bug, and the patch

Try `lookup(C, x)`. C's entry time is 50. Looking backwards on x's timeline finds time 10, value 2, which is A's binding.

That's wrong. C is in a different branch. C should see root's `x=1`.

The problem is that the timeline records when x was *set* but not when that setting stopped applying. A's binding of x was only live while the walk was inside A, from 10 to 40. After 40, root's value is in force again.

So whenever a node binds a variable, write **two** marks: the new value at its enter time, and the restored old value at its leave time.

```
timeline for x:   0 → 1        root sets x=1
                 10 → 2        A sets x=2
                 40 → 1        A's scope ends, x reverts to 1
                100 → unbound  root's scope ends

timeline for y:  20 → 7,  30 → unbound,  50 → 9,  60 → unbound
```

Now `lookup(C, x)` at time 50 looks backwards, finds the mark at 40, and returns 1. Correct.

And `lookup(A, y)` at time 10 looks backwards on y's timeline, finds nothing before 10, and returns unbound. Also correct, since y is only bound below and beside A.

That's the entire data structure. Two integers per node, two marks per bind, one sorted list per variable.

#### Why this is small

The per-node-dictionary version pays for **inheritance**. Every node re-materializes everything it can see, and that repetition is the quadratic blowup.

The timeline version never stores inheritance at all. B seeing `x=2` isn't a stored fact anywhere. It's the answer that falls out of a search over marks that A already paid for. The only writes happen at the moments a value actually changes.

So the total is two marks per bind. A path of depth n doing one bind per node produces 2n marks. Deep tree, wide tree, doesn't matter — you pay for changes, not for positions.

#### The one remaining wrinkle

I quietly assumed you could hand out times like 0, 10, 20, 50. That's easy when the tree already exists and you can walk it once.

But you wanted branches growing at the same time. So you might have A occupying 10 to 40, and later need to add a new child of root that belongs after A but before C at 50. Fine, use 45. Do that enough times and you run out of room between adjacent integers, and renumbering everything is expensive.

The escape is a small helper structure, called **order maintenance**, whose only job is: "give me a new label that sorts between these two," in constant amortized time. Internally it hands out numbers with gaps and quietly relabels small local chunks when a region gets crowded, so the cost stays flat on average. You treat it as a black box that provides labels you can compare with `<`.

That's the piece that makes this work for live, simultaneously-growing branches rather than only for a finished tree.

#### The cost summary

| | cost |
|---|---|
| space per node | 3 fields |
| space per bind | 2 marks |
| create a node | O(1) amortized (one label pair) |
| bind a variable | one lookup + two inserts |
| look up a variable | binary search on that variable's timeline |
| switching between branches | free, since nothing is undone or rebuilt |

That last row is the payoff over Prolog's trail. A trail gets O(1) lookups by keeping one mutable current environment, which means moving to a different branch requires unwinding and replaying. Here there is no current environment to unwind. Every node's view is queryable at any moment, in any order, because the answer was never stored in a place that had to be torn down.

---

## Question 5

> what is this system called?

### Answer

There isn't one clean name, because you've been shown a stack of three ideas that come from different literatures and are usually named separately.

**The whole thing** is a **fully persistent array**, sometimes **persistent array** or **persistent random-access map**. "Persistent" here means old versions stay readable after updates — as opposed to "ephemeral," where writing destroys the previous state. The specific construction with the timelines is **Dietz's persistent array** (Paul Dietz, *Fully Persistent Arrays*, WADS 1989). That's the paper that gives the O(1) amortized update and O(log log n) lookup.

Zooming out one more level, the general theory is **persistent data structures**, and the foundational paper is Driscoll, Sarnak, Sleator, and Tarjan, *Making Data Structures Persistent* (1989). Worth knowing the vocabulary distinctions:

- **partially persistent** — read any version, write only to the newest
- **fully persistent** — read *and* write any version, so history is a tree. This is your case: two branches both updating from a shared ancestor.
- **confluently persistent** — you can also merge two versions back together

**The individual pieces** have their own names:

- The enter/leave times are **Euler tour numbering**, or **interval labeling** / **tin-tout labeling**. The property that ancestry becomes an interval containment check is standard in competitive programming and in XML indexing, where it's known as **XML region encoding** or the **containment labeling scheme**.
- The "hand me a label between these two" black box is **order maintenance** or the **list labeling problem** (Dietz & Sleator, 1987).
- Looking backwards on a timeline is a **predecessor query**, and if you want the O(log log n) version, that's a **y-fast trie** or a **van Emde Boas tree**.

**Applied to unification**, which was your original framing, the name is **persistent union-find** — Conchon & Filliâtre, 2007. And the problem you posed, a tree of branches sharing binding environments, is exactly what the logic programming world calls **OR-parallel execution** and the **multiple binding environment problem**. That literature has its own catalogue of solutions: binding arrays, hashing windows, version vectors, time-stamping, stack copying.

If you want to say it in one breath to another engineer: *"a fully persistent map using Euler-tour interval labels and per-variable predecessor lists."* That tells them everything.

---

## Question 6

> wait what? is that the name for this tree-specific behavior?

### Answer

Yes. **"Fully persistent"** is precisely the term for the tree-shaped-version behavior. That's not an incidental adjective, it's the whole definition:

| term | shape of version history | what you can do |
|---|---|---|
| **ephemeral** | a single point | writing destroys the old state |
| **partially persistent** | a line | read any old version, write only to the newest |
| **fully persistent** | **a tree** | read *and* write any version, so updates branch |
| **confluently persistent** | a DAG | plus merge two versions back together |

The classification is literally by the shape of the version graph. When you update a version that already has a child, you create a *sibling*, and the history forks. Do that repeatedly and you get exactly the tree you described in your first message, with branches sharing ancestor environments and no backtracking required to reach a sibling.

So "fully persistent array" isn't a name for arrays-that-happen-to-be-used-in-a-tree. It's a name for *the tree behavior*, applied to arrays.

Worth noting where Prolog sits in that table: the trail-and-undo scheme is not fully persistent. Old versions exist only while you're on the path to them, and reaching a sibling means destroying the current state to rebuild the ancestor. Conchon and Filliâtre gave that its own name, **semi-persistent** (ESOP 2008): you may return to an ancestor version and update from there, but doing so invalidates the descendants you abandoned. Only one branch is alive at a time.

The gap between semi-persistent and fully persistent is exactly the thing you asked for.

---

## Question 7

> what does that mean "applied to arrays"

### Answer

It means "persistence" is a property you can bolt onto *any* data structure, and "array" is just which one we bolted it onto here.

Persistence answers one question: **when I update this thing, does the old version survive?** That question is orthogonal to what the thing is. So you get a grid:

| | list | array / map | union-find | balanced tree |
|---|---|---|---|---|
| **ephemeral** | linked list | mutable array | classic union-find | mutable AVL |
| **fully persistent** | any functional cons list | **Dietz's persistent array** | Conchon–Filliâtre | immutable AVL / HAMT |

The left column is what operations the structure offers. The row is whether history branches. Dietz's paper is one cell in that grid.

#### Why "array" specifically

Because that's the interface underneath your problem. Strip away the tree and each node needs exactly two operations:

```
get(var) -> value
set(var, value)
```

Indexed access by key, constant time, no ordering, no traversal. That's an array (or equivalently a map, if your variables aren't dense integers — the distinction doesn't matter for the technique).

Now add the tree requirement: `set` must not destroy what the parent sees, and siblings must not see each other's writes. That's persistence. Put them together and the thing you need is a fully persistent array. Dietz's construction is one way to build it — the one with Euler labels and per-variable timelines.

#### What "applied to" is ruling out

I said it to head off a specific misreading: that the technique is *about* trees, and arrays are the payload.

It's backwards. The array is the thing. The tree is the version history, which is what persistence *always* produces — it's the shape "fully persistent" names. So the phrase decomposes as:

- **array** — what you can do with it: get and set by index
- **fully persistent** — every version stays live and updatable, so history branches

The reason it fits your problem so exactly is that your two trees are the same tree. Your syntactic tree of nodes-introducing-bindings *is* the version history of one array. A node isn't a thing that has an environment; a node is a version of the environment. Once you see that, "I want a tree of shared binding environments" and "I want a fully persistent array" are the same sentence.

#### The generalization

You could apply persistence to something else if your nodes needed different operations. If they needed sorted iteration and range queries, you'd want a fully persistent balanced BST. If they needed priority-queue operations, a persistent heap. Same row of the grid, different column, different construction paper.

Your nodes need get and set. So: array.

---

## References

- Paul F. Dietz. *Fully Persistent Arrays.* WADS 1989.
- Paul F. Dietz and Daniel D. Sleator. *Two Algorithms for Maintaining Order in a List.* STOC 1987.
- James R. Driscoll, Neil Sarnak, Daniel D. Sleator, Robert E. Tarjan. *Making Data Structures Persistent.* JCSS 1989.
- Sylvain Conchon and Jean-Christophe Filliâtre. *A Persistent Union-Find Data Structure.* ML Workshop 2007.
- Sylvain Conchon and Jean-Christophe Filliâtre. *Semi-Persistent Data Structures.* ESOP 2008.
- Gopal Gupta and Bharat Jayaraman. *On Criteria for Or-Parallel Execution Models of Logic Programs.* NACLP 1990.
- Dan E. Willard. *Log-Logarithmic Worst-Case Range Queries are Possible in Space Θ(N).* (y-fast tries.) IPL 1983.
