package org.checkerframework.dataflow.cfg.node;

import java.util.Collection;
import java.util.Collections;

import org.checkerframework.dataflow.util.HashCodeUtils;

import org.checkerframework.javacutil.InternalUtils;

import com.sun.source.tree.Tree;
import com.sun.source.tree.Tree.Kind;

/**
 * A node for the unary 'nullchk' operation (generated by the Java compiler):
 *
 * <pre>
 *   &lt;*nullchk*&gt;<em>expression</em>
 * </pre>
 *
 * @author Stefan Heule
 * @author Charlie Garrett
 */
public class NullChkNode extends Node {

        protected Tree tree;
        protected Node operand;

        public NullChkNode(Tree tree, Node operand) {
            super(InternalUtils.typeOf(tree));
            assert tree.getKind() == Kind.OTHER;
            this.tree = tree;
            this.operand = operand;
        }

        public Node getOperand() {
            return operand;
        }

        @Override
        public Tree getTree() {
            return tree;
        }

        @Override
        public <R, P> R accept(NodeVisitor<R, P> visitor, P p) {
            return visitor.visitNullChk(this, p);
        }

        @Override
        public String toString() {
            return "(+ " + getOperand() + ")";
        }

        @Override
        public boolean equals(Object obj) {
            if (obj == null || !(obj instanceof NumericalPlusNode)) {
                return false;
            }
            NumericalPlusNode other = (NumericalPlusNode) obj;
            return getOperand().equals(other.getOperand());
        }

        @Override
        public int hashCode() {
            return HashCodeUtils.hash(getOperand());
        }

        @Override
        public Collection<Node> getOperands() {
            return Collections.singletonList(getOperand());
        }
    }
