#include <math.h>

#include "differentiator.h"
#include "base.h"
#include "const_strings.h"
#include "article.h"

const double EPSILON = 1e-12;

bool double_equal(double a, double b) {
    double diff = fabs(a - b);
    double max_ab = fmax(fabs(a), fabs(b));
    return diff <= EPSILON * max_ab;
}

function bool subtree_constant(const NODE_T *node) {
    if (!node) return false;
    if (node->type == VAR_T) return false;
    if (node->type == NUM_T) return true;
    bool left  = !node->left  || subtree_constant(node->left);
    bool right = !node->right || subtree_constant(node->right);
    return left && right;
}

function double eval_constant(const NODE_T *node) {
    if (!node) return 0.0;
    switch (node->type) {
        case NUM_T: return node->value.num;
        case OP_T: {
            double l = node->left ? eval_constant(node->left) : 0.0;
            double r = node->right ? eval_constant(node->right) : 0.0;
            switch (node->value.opr) {
                case ADD:  return l + r;
                case SUB:  return l - r;
                case MUL:  return l * r;
                case DIV:  return l / r;
                case POW:  return pow(l, r);
                case LOG:  return log(l) / log(r);
                case LN:   return log(l);
                case SIN:  return sin(l);
                case COS:  return cos(l);
                case TAN:  return tan(l);
                case CTG:  return 1.0 / tan(l);
                case ASIN: return asin(l);
                case ACOS: return acos(l);
                case ATAN: return atan(l);
                case ACTG: return atan(1.0 / l);
                case SQRT: return sqrt(l);
                case SINH: return sinh(l);
                case COSH: return cosh(l);
                case TANH: return tanh(l);
                case CTH:  return 1.0 / tanh(l);
                default:   return 0.0;
            }
        }
        default: return 0.0;
    }
}

function void replace_with_number(NODE_T *node, double value) {
    NODE_T *l = node->left;
    NODE_T *r = node->right;
    node->left = node->right = nullptr;
    node->type = NUM_T;
    node->value.num = value;
    node->elements = 0;
    destruct(l);
    destruct(r);
}

function bool fold_constants(NODE_T *node) {
    if (!node) return false;
    bool changed = false;
    if (node->left)  changed |= fold_constants(node->left);
    if (node->right) changed |= fold_constants(node->right);
    if (node->type == OP_T && subtree_constant(node)) {
        replace_with_number(node, eval_constant(node));
        changed = true;
    }
    return changed;
}

function bool is_number(const NODE_T *node, double value) {
    return node && node->type == NUM_T && double_equal(node->value.num, value);
}

function void adopt_child(NODE_T *node, NODE_T *keep, NODE_T *drop) {
    NODE_T *parent = node->parent;
    // fprintf(stderr, "adopt_child node=%p keep=%p drop=%p\n", (void *) node, (void *) keep, (void *) drop);
    if (keep) {
        destruct(drop);
    }
    if (!keep) {
        replace_with_number(node, 0.0);
        node->parent = parent;
        return;
    }
    NODE_T copy = *keep;
    *node = copy;
    node->parent = parent;
    if (node->left)  node->left->parent = node;
    if (node->right) node->right->parent = node;
    FREE(keep);
}

function bool simplify_neutral(NODE_T *node) {
    if (!node) return false;
    bool changed = false;
    if (node->left)  changed |= simplify_neutral(node->left);
    if (node->right) changed |= simplify_neutral(node->right);
    if (node->type != OP_T) return changed;
    NODE_T *l = node->left;
    NODE_T *r = node->right;
    switch (node->value.opr) {
        case MUL:
            if (is_number(l, 0.0) ||
                is_number(r, 0.0)) { replace_with_number(node, 0.0); changed = true; break; }
            if (is_number(l, 1.0)) { adopt_child(node, r, l);        changed = true; break; }
            if (is_number(r, 1.0)) { adopt_child(node, l, r);        changed = true; break; }
            break;
        case ADD:
            if (is_number(l, 0.0)) { adopt_child(node, r, l);        changed = true; break; }
            if (is_number(r, 0.0)) { adopt_child(node, l, r);        changed = true; break; }
            break;
        case SUB:
            if (is_number(r, 0.0)) { adopt_child(node, l, r);        changed = true; break; }
            break;
        case DIV:
            if (is_number(l, 0.0)) { replace_with_number(node, 0.0); changed = true; break; }
            if (is_number(r, 1.0)) { adopt_child(node, l, r);        changed = true; break; }
            break;
        case POW:
            if (is_number(r, 0.0)) { replace_with_number(node, 1.0); changed = true; break; }
            if (is_number(r, 1.0)) { adopt_child(node, l, r);        changed = true; break; }
            if (is_number(l, 1.0)) { replace_with_number(node, 1.0); changed = true; break; }
            break;
        default: break;
    }
    return changed;
}

function size_t recount_elements(NODE_T *node) {
    if (!node) return 0;
    size_t total = 0;
    if (node->left) { node->left ->parent = node; total += recount_elements(node->left ) + 1; }
    if (node->right){ node->right->parent = node; total += recount_elements(node->right) + 1; }
    node->elements = total;
    return total;
}

bool simplify_tree(EQ_TREE_T *eqtree) {
    if (!eqtree || !eqtree->root) return false;
    const EQ_TREE_T *prev_tree = differentiate_get_article_tree();
    differentiate_set_article_tree(eqtree);
    bool changed = false;
    do {
        changed = false;
        if (fold_constants(eqtree->root)) {
            recount_elements(eqtree->root);
            eqtree->root->parent = nullptr;
            changed = true;
        }
        if (simplify_neutral(eqtree->root)) {
            recount_elements(eqtree->root);
            eqtree->root->parent = nullptr;
            changed = true;
        }
    } while (changed);
    article_log_with_latex(eqtree, "\\bigskip\\hrule\\bigskip\nПутем несложных математических преобразований получим упрощенное выражение:");
    differentiate_set_article_tree(prev_tree);
    return changed;
}

