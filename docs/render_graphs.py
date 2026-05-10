"""Renders the Atlas event-graph diagrams as PNG images in docs/img/."""
import os
import textwrap
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import networkx as nx

OUT = os.path.join(os.path.dirname(__file__), "img")
os.makedirs(OUT, exist_ok=True)

# ── colour palette ────────────────────────────────────────────────────────────
C_EVENT    = "#4e8fd6"   # blue
C_HANDLER  = "#e8a838"   # amber
C_ENTITY   = "#5bb86a"   # green
C_EXTERNAL = "#9b59b6"   # purple
C_WARN     = "#e05252"   # red  (⚠ nodes)

FONT = "monospace"


def node_colour(label: str) -> str:
    if label.startswith("[["):
        return C_HANDLER
    if label.startswith("("):
        return C_ENTITY
    if label.startswith("external"):
        return C_EXTERNAL
    if "⚠" in label:
        return C_WARN
    return C_EVENT


def clean(label: str) -> str:
    """Strip Mermaid bracket syntax for display."""
    for ch in "[]()":
        label = label.replace(ch, "")
    return label.strip()


def wrap(text: str, width: int = 22) -> str:
    return "\n".join(textwrap.wrap(text, width))


# ── generic renderer ──────────────────────────────────────────────────────────

def render(
    title: str,
    nodes: dict[str, str],   # id → display label (may contain [[ ]] ( ) etc.)
    edges: list[tuple[str, str, str]],  # (src_id, dst_id, edge_label)
    filename: str,
    figsize: tuple[float, float] = (14, 9),
    prog: str = "dot",
) -> None:
    G = nx.DiGraph()
    for nid in nodes:
        G.add_node(nid)
    for src, dst, lbl in edges:
        G.add_edge(src, dst, label=lbl)

    try:
        pos = nx.nx_agraph.graphviz_layout(G, prog=prog)
    except Exception:
        # Fall back to spring layout if graphviz not available
        pos = nx.spring_layout(G, seed=42, k=2.2)

    colours = [node_colour(nodes[n]) for n in G.nodes()]
    labels  = {n: wrap(clean(nodes[n])) for n in G.nodes()}

    fig, ax = plt.subplots(figsize=figsize)
    ax.set_facecolor("#1e1e2e")
    fig.patch.set_facecolor("#1e1e2e")
    ax.set_title(title, color="white", fontsize=14, fontweight="bold", pad=12)
    ax.axis("off")

    nx.draw_networkx_nodes(
        G, pos, ax=ax,
        node_color=colours,
        node_size=2800,
        alpha=0.92,
    )
    nx.draw_networkx_labels(
        G, pos, labels=labels, ax=ax,
        font_size=6.5, font_color="white", font_family=FONT,
    )
    nx.draw_networkx_edges(
        G, pos, ax=ax,
        edge_color="#aaaaaa",
        arrows=True,
        arrowsize=18,
        arrowstyle="-|>",
        connectionstyle="arc3,rad=0.08",
        width=1.4,
        min_source_margin=22,
        min_target_margin=22,
    )
    edge_labels = {(s, d): l for s, d, l in edges if l}
    nx.draw_networkx_edge_labels(
        G, pos, edge_labels=edge_labels, ax=ax,
        font_size=6, font_color="#ffdd99", font_family=FONT,
        bbox=dict(boxstyle="round,pad=0.2", fc="#2a2a3e", ec="none", alpha=0.7),
    )

    legend = [
        mpatches.Patch(color=C_EVENT,    label="event"),
        mpatches.Patch(color=C_HANDLER,  label="handler  [[ ]]"),
        mpatches.Patch(color=C_ENTITY,   label="entity  ( )"),
        mpatches.Patch(color=C_WARN,     label="⚠  gap"),
        mpatches.Patch(color=C_EXTERNAL, label="external"),
    ]
    ax.legend(handles=legend, loc="lower left", framealpha=0.4,
              labelcolor="white", facecolor="#2a2a3e", edgecolor="none",
              fontsize=8)

    path = os.path.join(OUT, filename)
    fig.tight_layout()
    fig.savefig(path, dpi=150, bbox_inches="tight",
                facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  wrote {path}")


# ═══════════════════════════════════════════════════════════════════════════════
# 1a  Sim Lifecycle – Start
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "1a · Sim Lifecycle — Start",
    {
        "ext":            "external\n(first start)",
        "e_starting":     "[sim_starting_event]",
        "h_starter":      "[[sim_starting\nsim_starter_EH]]",
        "ent_starter":    "(sim_starter)",
        "ent_iga":        "(initial_goal\nactivator)",
        "e_iga":          "[initial_goal\nactivating_event × N]",
        "h_iga_bridge":   "[[initial_goal_activating\ngoal_activated_bridge_EH]]",
        "e_activated":    "[goal_activated_event]\n⚠ no handler",
        "e_started":      "[sim_started_event]\n⚠ never produced",
        "h_started_nmug": "[[sim_started\nno_more_unit_goals\nbridge_EH]]",
        "e_nmug":         "[no_more_unit_goals\nevent]",
    },
    [
        ("ext",         "e_starting",     ""),
        ("e_starting",  "h_starter",      ""),
        ("h_starter",   "ent_starter",    "calls start()"),
        ("ent_starter", "ent_starter",    "push trail"),
        ("ent_starter", "ent_iga",        "activate_initial_goals()"),
        ("ent_iga",     "e_iga",          ""),
        ("e_iga",       "h_iga_bridge",   ""),
        ("h_iga_bridge","e_activated",    ""),
        ("e_started",   "h_started_nmug", ""),
        ("h_started_nmug", "e_nmug",      ""),
    ],
    "01a_sim_lifecycle_start.png",
    figsize=(12, 10),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 1b  Sim Lifecycle – Stop
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "1b · Sim Lifecycle — Stop",
    {
        "e_stopping":    "[sim_stopping_event]",
        "h_cancelled":   "[[sim_stopping\nsim_cancelled\nbridge_EH]]",
        "e_cancelled":   "[sim_cancelled_event]",
        "h_stopper":     "[[sim_stopping\nsim_stopper_EH]]",
        "ent_stopper":   "(sim_stopper)",
        "e_clearing":    "[goal_stores\nclearing_event]",
        "h_cl_bridge":   "[[goal_stores_clearing\ncleared_bridge_EH]]",
        "e_cleared":     "[goal_stores\ncleared_event]",
        "h_cl_stopper":  "[[goal_stores_cleared\nsim_stopper_EH]]",
        "e_stopped":     "[sim_stopped_event]",
        "h_stopped_start":"[[sim_stopped\nsim_starting\nbridge_EH]]",
        "h_stopped_reset":"[[sim_stopped\nsim_cancellation\nreset_bridge_EH]]",
        "e_starting":    "[sim_starting_event]",
        "e_reset":       "[sim_cancellation\nreset_event]",
    },
    [
        ("e_stopping",   "h_cancelled",    ""),
        ("e_stopping",   "h_stopper",      ""),
        ("h_cancelled",  "e_cancelled",    ""),
        ("h_stopper",    "ent_stopper",    "calls init_stop()"),
        ("ent_stopper",  "ent_stopper",    "trail.pop()\n+derive_lemma()"),
        ("ent_stopper",  "e_clearing",     ""),
        ("e_clearing",   "h_cl_bridge",    ""),
        ("h_cl_bridge",  "e_cleared",      ""),
        ("e_cleared",    "h_cl_stopper",   ""),
        ("h_cl_stopper", "ent_stopper",    "calls finish_stop()"),
        ("ent_stopper",  "e_stopped",      "cdcl.learn(lemma)"),
        ("e_stopped",    "h_stopped_start",""),
        ("e_stopped",    "h_stopped_reset",""),
        ("h_stopped_start","e_starting",   ""),
        ("h_stopped_reset","e_reset",      ""),
    ],
    "01b_sim_lifecycle_stop.png",
    figsize=(13, 10),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 2a  Solving Loop – Decider
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "2a · Solving Loop — Decider",
    {
        "e_nmug":          "[no_more_unit\ngoals_event]",
        "h_decider":       "[[decider\nno_more_unit_goals_EH]]\n★ cancellable",
        "h_repeater":      "[[no_more_unit_goals\nrepeater_EH]]\n★ cancellable",
        "ent_decider":     "(decider)",
        "e_deciding":      "[deciding_event]",
        "h_dec_store":     "[[decision_memory\ndeciding_EH]]",
        "h_dec_decided":   "[[deciding_decided\nbridge_EH]]",
        "e_decided":       "[decided_event]",
        "h_res_decided":   "[[goal_resolver\ndecided_EH]]\n★ cancellable",
        "ent_resolver":    "(goal_resolver)",
    },
    [
        ("e_nmug",        "h_decider",      ""),
        ("e_nmug",        "h_repeater",     ""),
        ("h_repeater",    "e_nmug",         "lower priority"),
        ("h_decider",     "ent_decider",    "calls decide()"),
        ("ent_decider",   "e_deciding",     ""),
        ("e_deciding",    "h_dec_store",    ""),
        ("e_deciding",    "h_dec_decided",  ""),
        ("h_dec_decided", "e_decided",      ""),
        ("e_decided",     "h_res_decided",  ""),
        ("h_res_decided", "ent_resolver",   "calls resolve(rl)"),
    ],
    "02a_solving_loop_decider.png",
    figsize=(13, 8),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 2b  Solving Loop – Unit Goals fast path
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "2b · Solving Loop — Unit Goal Fast Path",
    {
        "e_unit":     "[goal_unit_event]",
        "h_unit":     "[[goal_resolver\ngoal_unit_EH]]\n★ cancellable",
        "ent_res":    "(goal_resolver)",
    },
    [
        ("e_unit", "h_unit",  ""),
        ("h_unit", "ent_res", "calls resolve(rl)"),
    ],
    "02b_solving_loop_unit_goal.png",
    figsize=(7, 4),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 3a  Goal Resolution
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "3a · Goal Resolution",
    {
        "ent_res":       "(goal_resolver)",
        "e_resolving":   "[goal_resolving\nevent]",
        "h_res_bridge":  "[[goal_resolving\ngoal_resolved\nbridge_EH]]",
        "e_resolved":    "[goal_resolved\nevent]",
        "h_store_res":   "[[resolution_store\ngoal_resolved_EH]]",
        "e_activating":  "[goal_activating\nevent]",
        "h_act_bridge":  "[[goal_activating\ngoal_activated\nbridge_EH]]",
        "e_activated":   "[goal_activated_event]\n⚠ no handler",
        "e_deactivating":"[goal_deactivating\nevent]",
        "h_deact_bridge":"[[goal_deactivating\ngoal_deactivated\nbridge_EH]]",
        "e_deactivated": "[goal_deactivated\nevent]",
        "h_det":         "[[goal_deactivated\nactive_goals_empty\ndetector_EH]]",
        "ent_det":       "(active_goals\nempty_detector)",
        "e_empty":       "[active_goals\nempty_event]",
        "h_empty_solved":"[[active_goals_empty\nsolved_bridge_EH]]\n★ cancellable",
        "e_solved":      "[solved_event]",
        "h_solved_stop": "[[solved_sim_stopping\nbridge_EH]]",
        "e_stopping":    "[sim_stopping_event]",
    },
    [
        ("ent_res",       "e_resolving",    ""),
        ("ent_res",       "e_activating",   ""),
        ("ent_res",       "e_deactivating", ""),
        ("e_resolving",   "h_res_bridge",   ""),
        ("h_res_bridge",  "e_resolved",     ""),
        ("e_resolved",    "h_store_res",    ""),
        ("e_activating",  "h_act_bridge",   ""),
        ("h_act_bridge",  "e_activated",    ""),
        ("e_deactivating","h_deact_bridge", ""),
        ("h_deact_bridge","e_deactivated",  ""),
        ("e_deactivated", "h_det",          ""),
        ("h_det",         "ent_det",        "calls goal_deactivated()"),
        ("ent_det",       "e_empty",        ""),
        ("e_empty",       "h_empty_solved", ""),
        ("h_empty_solved","e_solved",       ""),
        ("e_solved",      "h_solved_stop",  ""),
        ("h_solved_stop", "e_stopping",     ""),
    ],
    "03a_goal_resolution.png",
    figsize=(13, 12),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 3b  Empty Candidates → Conflict
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "3b · Empty Candidates → Conflict",
    {
        "e_cand_empty":  "[goal_candidates\nempty_event]",
        "h_conf_bridge": "[[goal_candidates_empty\nconflicted_bridge_EH]]",
        "e_conflicted":  "[conflicted_event]",
        "h_conf_stop":   "[[conflicted_sim_stopping\nbridge_EH]]",
        "e_stopping":    "[sim_stopping_event]",
    },
    [
        ("e_cand_empty",  "h_conf_bridge", ""),
        ("h_conf_bridge", "e_conflicted",  ""),
        ("e_conflicted",  "h_conf_stop",   ""),
        ("h_conf_stop",   "e_stopping",    ""),
    ],
    "03b_empty_candidates.png",
    figsize=(8, 5),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 4  CDCL / Avoidance
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "4 · CDCL / Avoidance",
    {
        "ent_cdcl":      "(cdcl)\n⚠ updated() never called",
        "e_av_unit":     "[avoidance_unit\nevent]",
        "e_av_empty":    "[avoidance_empty\nevent]",
        "h_router":      "[[router_avoidance\nunit_EH]]\n★ cancellable",
        "h_av_empty":    "[[avoidance_empty_EH]]\n★ cancellable",
        "e_conflicted":  "[conflicted_event]",
        "e_refuted":     "[refuted_event]\n⚠ no handler",
    },
    [
        ("ent_cdcl",   "e_av_unit",   "via updated()\n— unreachable"),
        ("ent_cdcl",   "e_av_empty",  "via updated()\n— unreachable"),
        ("e_av_unit",  "h_router",    ""),
        ("e_av_empty", "h_av_empty",  ""),
        ("h_av_empty", "e_conflicted",""),
        ("h_av_empty", "e_refuted",   ""),
    ],
    "04_cdcl_avoidance.png",
    figsize=(11, 7),
)

# ═══════════════════════════════════════════════════════════════════════════════
# 5  Store Clearing
# ═══════════════════════════════════════════════════════════════════════════════
render(
    "5 · Store Clearing",
    {
        "e_gscl":   "[goal_stores\nclearing_event]",
        "h1":       "[[active_goal_store\nclearing_EH]]",
        "h2":       "[[decision_memory\nclearing_EH]]",
        "h3":       "[[goal_candidates_store\nclearing_EH]]",
        "h4":       "[[goal_expr_store\nclearing_EH]]",
        "h5":       "[[goal_weight_store\nclearing_EH]]",
        "h6":       "[[inactive_goal_store\nclearing_EH]]",
        "h7":       "[[resolution_store\nclearing_EH]]",
        "h8":       "[[goal_stores_clearing\ncleared_bridge_EH]]",
        "e_gsclrd": "[goal_stores\ncleared_event]",
    },
    [
        ("e_gscl", "h1", ""), ("e_gscl", "h2", ""), ("e_gscl", "h3", ""),
        ("e_gscl", "h4", ""), ("e_gscl", "h5", ""), ("e_gscl", "h6", ""),
        ("e_gscl", "h7", ""), ("e_gscl", "h8", ""),
        ("h8", "e_gsclrd", ""),
    ],
    "05_store_clearing.png",
    figsize=(13, 7),
)

print("Done — all images written to docs/img/")
