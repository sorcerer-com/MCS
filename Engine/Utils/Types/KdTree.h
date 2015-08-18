// KdTree.h
#pragma once

#include <vector>

#include "Vector3.h"

using namespace std;

namespace MyEngine {

    template <typename Vector, typename IndexType = int, typename DistanceType = float>
    struct KdTree
    {
    protected:
        struct Node
        {
            IndexType axis;
            DistanceType splitPos;
            union {
                Node* children;
                vector<IndexType>* indices;
            } data;
            
            ~Node()
            {
                if (axis == -1)
                    delete this->data.indices;
                else
                    delete[] this->data.children;
            }
        };
        
        struct BoundingBox
        {
            Vector min;
            Vector max;
        };

        IndexType dim;
        IndexType max_leaf_size;
        IndexType max_tree_depth;
        IndexType max_depth_best_split;

        Node* root_node;
        BoundingBox bounding_box;

    public:
        KdTree(const IndexType& dimension, const IndexType& max_leaf_size = 20, const IndexType& max_tree_depth = 64, const IndexType& max_depth_best_split = 5)
        {
            this->dim = dimension;
            this->max_leaf_size = max_leaf_size;
            this->max_tree_depth = max_tree_depth;
            this->max_depth_best_split = max_depth_best_split;

            this->root_node = NULL;
        }

        ~KdTree()
        {
            delete root_node;
        }


        template <typename Func>
        void build(const IndexType& size, const Func& getElement)
        {
            this->clear();

            vector<IndexType> indices(size);
            for (IndexType i = 0; i < size; i++)
            {
                const Vector& elem = getElement(i);
                indices[i] = i;

                // compute bounding box
                for (IndexType j = 0; j < this->dim; j++)
                {
                    this->bounding_box.min[j] = min(this->bounding_box.min[j], elem[j]);
                    this->bounding_box.max[j] = max(this->bounding_box.max[j], elem[j]);
                }
            }

            this->root_node = new Node();
            this->build(*this->root_node, indices, getElement, this->bounding_box, 0);
        }

        template <typename Func>
        void insert(const IndexType& index, const Func& getElement)
        {
            const Vector& elem = getElement(index);
            if (this->root_node == NULL) // first element
            {
                this->root_node = new Node();
                this->root_node->axis = -1;
                this->root_node->splitPos = 0.0f;
                this->root_node->data.indices = new vector<IndexType>();
                this->root_node->data.indices->push_back(index);

                this->bounding_box.min = elem;
                this->bounding_box.max = elem;
            }
            else
            {
                for (IndexType j = 0; j < this->dim; j++)
                {
                    this->bounding_box.min[j] = min(this->bounding_box.min[j], elem[j]);
                    this->bounding_box.max[j] = max(this->bounding_box.max[j], elem[j]);
                }
                this->insert(*this->root_node, index, getElement, this->bounding_box, 0);
            }
        }

        template <typename Func>
        void find_nearest(const Vector& element, const Func& getElement, const IndexType& num_closest, vector<IndexType>& out_indices) const
        {
            if (this->root_node == NULL)
                return;

            this->find_nearest(*this->root_node, element, getElement, num_closest, out_indices);
        }

        template <typename Func>
        void find_range(const Vector& element, const Func& getElement, const DistanceType& range, vector<IndexType>& out_indices, bool sorted = true) const
        {
            if (this->root_node == NULL)
                return;

            this->find_range(*this->root_node, element, getElement, range, out_indices);
            
            if (sorted)
            {
                sort(out_indices.begin(), out_indices.end(), [&](const IndexType& a, const IndexType& b) -> bool
                {
                    return distance(getElement(a), element, this->dim) < distance(getElement(b), element, this->dim);
                });
            }
        }

        inline void clear()
        {
            if (this->root_node)
                delete this->root_node;
            this->root_node = NULL;
            this->bounding_box = BoundingBox();
        }

    private:
        template <typename Func>
        inline void build(Node& node, const vector<IndexType>& indices, const Func& getElement, const BoundingBox& bbox, const IndexType& depth)
        {
            if (indices.size() < this->max_leaf_size || depth > this->max_tree_depth) // there isn't a lot of indices, put them in leaf
            {
                node.axis = -1;
                node.splitPos = 0.0f;
                node.data.indices = new vector<IndexType>(indices);
            }
            else // split the node
            {
                IndexType splitAxis = longestAxis(bbox, this->dim);
                DistanceType ls = bbox.min[splitAxis];
                DistanceType rs = bbox.max[splitAxis];
                DistanceType bestSplit = 0.0f;
                if (depth > this->max_depth_best_split) // get the middle of the axis
                    bestSplit = (ls + rs) * 0.5f;
                else // find the best split position
                {
                    DistanceType lasMid = 0.0f;
                    for (IndexType i = 0; i < 10; i++)
                    {
                        DistanceType mid = (ls + rs) * 0.5f;
                        BoundingBox left, right;
                        split(bbox, splitAxis, mid, left, right);
                        IndexType tLeft = 0, tRight = 0;
                        for (IndexType idx : indices)
                        {
                            const Vector& elem = getElement(idx);
                            if (left.min < elem && elem < left.max)
                                tLeft++;
                            else if (right.min < elem && elem < right.max)
                                tRight++;
                        }

                        if (tLeft == 0 || tRight == 0)
                            lasMid = mid;
                        else if (lasMid != 0.0f)
                        {
                            ls = rs = lasMid;
                            break;
                        }

                        if (tLeft < tRight)
                            ls = mid;
                        else
                            rs = mid;
                    }
                    bestSplit = (ls + rs) * 0.5f;
                }
                node.axis = splitAxis;
                node.splitPos = bestSplit;
                node.data.children = new Node[2];

                BoundingBox left, right;
                split(bbox, splitAxis, bestSplit, left, right);
                vector<IndexType> leftIndices, rightIndices;
                for (IndexType idx : indices)
                {
                    const Vector& elem = getElement(idx);
                    if (inside(left, elem))
                        leftIndices.push_back(idx);
                    else if (inside(right, elem))
                        rightIndices.push_back(idx);
                }
                build(node.data.children[0], leftIndices, getElement, left, depth + 1);
                build(node.data.children[1], rightIndices, getElement, right, depth + 1);
            }
        }

        template <typename Func>
        inline void insert(Node& node, const IndexType& index, const Func& getElement, const BoundingBox& bbox, const IndexType& depth)
        {
            if (node.axis == -1) // leaf
            {
                if (node.data.indices->size() < this->max_leaf_size || depth > this->max_tree_depth) // there is enought "space"
                {
                    node.data.indices->push_back(index);
                }
                else // split the node
                {
                    vector<IndexType> indices = *node.data.indices;
                    delete node.data.indices;
                    build(node, indices, getElement, bbox, depth);
                }
            }
            else
            {
                BoundingBox left, right;
                split(bbox, node.axis, node.splitPos, left, right);
                const Vector& elem = getElement(index);
                if (inside(left, elem))
                    insert(node.data.children[0], index, getElement, left, depth + 1);
                else if (inside(right, elem))
                    insert(node.data.children[1], index, getElement, right, depth + 1);
            }
        }

        template <typename Func>
        inline void find_nearest(const Node& node, const Vector& element, const Func& getElement, const IndexType& num_closest, vector<IndexType>& out_indices) const
        {
            if (node.axis == -1) // leaf - from the elements search for closest num_closest elements
            {
                // add elements in out_indices and remove furthest
                for (IndexType idx : *node.data.indices)
                {
                    out_indices.push_back(idx);
                    sort(out_indices.begin(), out_indices.end(), [&](const IndexType& a, const IndexType& b) -> bool
                    {
                        const DistanceType& distA = distance(getElement(a), element, this->dim);
                        const DistanceType& distB = distance(getElement(b), element, this->dim);
                        if (abs(distA - distB) < 0.001f) // if they have same distance sort by index
                            return a < b;
                        else
                            return distA < distB;
                    });

                    if (out_indices.size() > num_closest)
                        out_indices.pop_back();
                }
            }
            else
            {
                DistanceType delta = element[node.axis] - node.splitPos;
                find_nearest((delta <= 0.0f ? node.data.children[0] : node.data.children[1]), element, getElement, num_closest, out_indices);
                if (out_indices.size() < num_closest || abs(delta) < distance(element, getElement(*out_indices.rbegin()), this->dim))
                    find_nearest((delta <= 0.0f ? node.data.children[1] : node.data.children[0]), element, getElement, num_closest, out_indices);
            }
        }

        template <typename Func>
        inline void find_range(const Node& node, const Vector& element, const Func& getElement, const DistanceType& range, vector<IndexType>& out_indices) const
        {
            if (node.axis == -1) // leaf - from the elements search for closest num_closest elements
            {
                // add elements in out_indices and remove furthest
                for (IndexType idx : *node.data.indices)
                {
                    if (distance(element, getElement(idx), this->dim) < range)
                        out_indices.push_back(idx);
                }
            }
            else
            {
                DistanceType delta = element[node.axis] - node.splitPos;
                find_range((delta <= 0.0f ? node.data.children[0] : node.data.children[1]), element, getElement, range, out_indices);
                if (abs(delta) < range)
                    find_range((delta <= 0.0f ? node.data.children[1] : node.data.children[0]), element, getElement, range, out_indices);
            }
        }


        inline static IndexType longestAxis(const BoundingBox& bbox, const IndexType& dim)
        {
            IndexType axis = 0;
            DistanceType maxAxis = bbox.max[0] - bbox.min[0];
            for (IndexType i = 1; i < dim; i++)
            {
                DistanceType temp = bbox.max[i] - bbox.min[i];
                if (temp > maxAxis)
                {
                    maxAxis = temp;
                    axis = i;
                }
            }
            return axis;
        }

        inline static void split(const BoundingBox& bbox, const IndexType& axis, const DistanceType& splitPos, BoundingBox& left, BoundingBox& right)
        {
            left = bbox;
            right = bbox;
            left.max[axis] = splitPos;
            right.min[axis] = splitPos;
        }

        inline static bool inside(const BoundingBox& bbox, const Vector& elem)
        {
            return bbox.min <= elem && elem <= bbox.max;
        }

        inline static DistanceType distance(const Vector& a, const Vector& b, const IndexType& dim)
        {
            DistanceType dist = 0.0f;
            for (IndexType i = 0; i < dim; i++)
            {
                DistanceType delta = a[i] - b[i];
                dist += delta * delta;
            }
            return sqrt(dist);
        }

    };

}