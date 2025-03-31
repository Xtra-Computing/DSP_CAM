import numpy as np
from collections import defaultdict
import math
import os
import networkx as nx

def calculate_triangles(preprocessed_edges):
    """
    Calculate the total number of triangles in the graph.
    Parameters:
        preprocessed_edges: numpy array of shape (num_edges, 2) representing edges.
    Returns:
        total_triangles: Total number of triangles in the graph.
    """
    # Create a networkx graph
    G = nx.Graph()
    G.add_edges_from(preprocessed_edges)
    # Calculate the number of triangles for each node
    triangle_counts = nx.triangles(G)
    # Sum up the triangle counts and divide by 3 (each triangle is counted for 3 nodes)
    total_triangles = sum(triangle_counts.values()) // 3
    return total_triangles

def count_triangles_from_csr(new_row, new_col, duplicated_edges):
    """
    Count the number of triangles in the graph based on CSR representation and edges.
    Parameters:
        new_row: CSR row array (size: num_vertices + 1).
        new_col: CSR column array (size: number of edges).
        duplicated_edges: List of edges in the graph (e.g., virtualized edges).
    Returns:
        total_triangles: Total number of triangles in the graph.
    """
    total_triangles = 0

    for src, dst in duplicated_edges:
        # Retrieve adjacency lists for src and dst
        src_start, src_end = new_row[src], new_row[src + 1]
        dst_start, dst_end = new_row[dst], new_row[dst + 1]
        src_adj = set(new_col[src_start:src_end])
        dst_adj = set(new_col[dst_start:dst_end])

        # Calculate intersection size
        common_neighbors = src_adj.intersection(dst_adj)
        total_triangles += len(common_neighbors)

    return total_triangles


def save_to_file(filename, data):
    """
    Save data to a text file.
    Parameters:
        filename: Name of the file to save.
        data: Data to save (can be list, numpy array, or dictionary).
    """
    with open(filename, "w") as f:
        if isinstance(data, dict):
            for key, value in data.items():
                f.write(f"{key} -> {value}\n")
        elif isinstance(data, list) or isinstance(data, np.ndarray):
            for item in data:
                if isinstance(item, (tuple, list, np.ndarray)) and len(item) == 2:  # Edge list
                    f.write(f"{item[0]} {item[1]}\n")
                else:
                    f.write(f"{item}\n")
        else:
            f.write(str(data) + "\n")

def save_to_file_with_prefix(output_dir, dataset_name, suffix, data):
    """
    Save data to a file with a prefixed name and store it in the specified directory.
    Parameters:
        output_dir: Directory to save the files.
        dataset_name: Name of the dataset file (e.g., 'amazon0302').
        suffix: Suffix for the file name (e.g., 'row.txt').
        data: Data to save (list, numpy array, or dictionary).
    """
    os.makedirs(output_dir, exist_ok=True)  # Ensure the directory exists
    file_path = os.path.join(output_dir, f"{dataset_name}_{suffix}")
    save_to_file(file_path, data)

def preprocess_edge_list(filename):
    """
    Preprocess the edge list file.
    Parameters:
        filename: Path to the file containing the edge list.
    Returns:
        txt_array: Preprocessed numpy array of edges.
    """
    print("Load data from hard-disk ... ")
    txt_array_t = np.int64(np.loadtxt(filename))
    txt_array = txt_array_t[:, :2]

    print("Delete 'sumdegree = 1' edges ... ")
    outdegree = np.bincount(txt_array[:, 0])
    indegree = np.bincount(txt_array[:, 1])
    sumdegree = np.zeros(txt_array.shape[0])
    padded_outdegree = np.pad(outdegree, (0, txt_array.shape[0] - len(outdegree)), mode='constant', constant_values=0)
    padded_indegree = np.pad(indegree, (0, txt_array.shape[0] - len(indegree)), mode='constant', constant_values=0)
    sumdegree = sumdegree + padded_indegree + padded_outdegree

    indices_to_delete = [i for i in range(txt_array.shape[0]) if sumdegree[txt_array[i][0]] == 1]
    txt_array = np.delete(txt_array, indices_to_delete, axis=0)

    print("Sort and clean edges ... ")
    for i in range(txt_array.shape[0]):
        if txt_array[i][0] >= txt_array[i][1]:
            txt_array[i][0], txt_array[i][1] = txt_array[i][1], txt_array[i][0]

    txt_array = txt_array[np.lexsort([txt_array.T[1]])]
    txt_array = txt_array[np.lexsort([txt_array.T[0]])]
    txt_array = txt_array[txt_array[:, 0] != txt_array[:, 1]]
    txt_array = np.unique(txt_array, axis=0)

    print("Preprocessing complete.")
    return txt_array

def process_graph_with_virtual_nodes(edge_list, max_length):
    """
    Process the edge list to handle large adjacency lists by creating virtual nodes.
    Parameters:
        edge_list: numpy array of shape (num_edges, 2) representing edges.
        max_length: Maximum allowed adjacency list length per vertex.
    Returns:
        duplicated_edges: Duplicated edge list after handling virtual nodes.
        virtual_node_mapping: Mapping from all vertices (original and virtual) to unique IDs.
        virtual_node_counts: Count of virtual nodes for each original vertex.
        node_mapping: Mapping from original vertex ID to remapped IDs.
    """
    # Step 1: Build adjacency list
    adj_list = defaultdict(list)
    for src, dst in edge_list:
        adj_list[src].append(dst)
        adj_list[dst]  # Ensure destination vertices are included

    # Step 2: Initialize virtual_node_counts for all vertices
    num_vertices = np.max(edge_list) + 1
    virtual_node_counts = np.ones(num_vertices, dtype=int)

    for vertex, neighbors in adj_list.items():
        if len(neighbors) > 0:
            virtual_node_counts[vertex] = math.ceil(len(neighbors) / max_length)

    # Step 3: Remap graph IDs to consecutive integers
    node_mapping = {node: idx for idx, node in enumerate(range(num_vertices))}  # Includes all vertices
    remapped_edge_list = [(node_mapping[src], node_mapping[dst]) for src, dst in edge_list]

    # Step 4: Generate virtual nodes and assign consecutive IDs
    virtual_node_mapping = {}
    current_id = 0

    # Process all nodes, including isolated ones
    for node in range(num_vertices):
        num_virtual_nodes = virtual_node_counts[node]
        for v in range(num_virtual_nodes):
            virtual_node_mapping[f"{node}_v{v}"] = current_id
            current_id += 1

    # Step 5: Duplicate edges based on virtual nodes
    new_edges = []
    for src, dst in remapped_edge_list:
        src_virtual_count = virtual_node_counts[src]
        dst_virtual_count = virtual_node_counts[dst]

        for src_v in range(src_virtual_count):
            for dst_v in range(dst_virtual_count):
                new_edges.append(
                    (
                        virtual_node_mapping[f"{src}_v{src_v}"],
                        virtual_node_mapping[f"{dst}_v{dst_v}"],
                    )
                )

    # Step 6: Sort the edge list
    duplicated_edges = sorted(new_edges, key=lambda edge: (edge[0], edge[1]))
    
    # Step 7: remove the last edge of each source
    duplicated_edges_array = np.array(duplicated_edges)
    unique_sources, counts = np.unique(duplicated_edges_array[:, 0], return_counts=True)
    remove_indices = np.cumsum(counts) - 1
    mask = np.ones(len(duplicated_edges_array), dtype=bool)
    mask[remove_indices] = False
    
    return duplicated_edges_array[mask], virtual_node_mapping, virtual_node_counts, node_mapping


def edgelist_to_csr(edge_list, num_vertices):
    adj_list = defaultdict(list)
    for src, dst in edge_list:
        adj_list[src].append(dst)

    row = [0] * (num_vertices + 1)
    col = []

    for vertex in range(num_vertices):
        neighbors = adj_list[vertex]
        row[vertex + 1] = row[vertex] + len(neighbors)
        col.extend(neighbors)

    return np.array(row), np.array(col)

def process_csr_with_virtual_nodes(row, col, max_length):
    """
    Process CSR row and col files to handle vertices with large adjacency lists.
    Parameters:
        row: CSR row array (size: num_vertices + 1).
        col: CSR column array (size: number of edges).
        max_length: maximum allowed adjacency list length per vertex.
    Returns:
        new_row: updated CSR row array with virtual nodes.
        new_col: updated CSR column array with remapped vertex IDs.
        node_mapping: mapping from original vertex ID to new IDs (including virtual nodes).
        row_2: additional row array of size 2 * num_vertices for cycle access.
    """
    num_vertices = len(row) - 1
    new_row = []
    new_col = []
    node_mapping = {}
    current_id = 0

    for vertex in range(num_vertices):
        start = row[vertex]
        end = row[vertex + 1]
        neighbors = col[start:end]

        # Ensure all vertices, including isolated ones, are in the mapping
        if len(neighbors) == 0:
            node_mapping[f"{vertex}_v0"] = current_id
            new_row.append(len(new_col))
            current_id += 1
            continue

        # Determine the number of virtual nodes required
        num_virtual_nodes = math.ceil(len(neighbors) / max_length)
        for v in range(num_virtual_nodes):
            virtual_id = current_id
            node_mapping[f"{vertex}_v{v}"] = virtual_id
            new_row.append(len(new_col))  # Start index for this virtual node

            # Assign a portion of the neighbors to this virtual node
            start_idx = v * max_length
            end_idx = min((v + 1) * max_length, len(neighbors))
            new_col.extend(neighbors[start_idx:end_idx])

            current_id += 1

    # Finalize the row array
    new_row.append(len(new_col))

    # Remap col file based on node_mapping
    remapped_col = [
        node_mapping[f"{dst}_v0"] if f"{dst}_v0" in node_mapping else dst for dst in new_col
    ]

    # Generate row_2 array for cycle access
    row_2 = []
    for i in range(len(new_row) - 1):
        row_2.append(new_row[i])  # Current vertex offset
        row_2.append(new_row[i + 1])  # Next vertex offset

    return np.array(new_row), np.array(remapped_col), node_mapping, np.array(row_2)


def process_multiple_datasets(file_list, max_length, input_dir="../datasets", output_dir="./generated"):
    for file in file_list:
        full_path = os.path.join(input_dir, file)
        dataset_name = os.path.splitext(os.path.basename(file))[0]
        print(f"\nProcessing file: {full_path}")

        preprocessed_edges = preprocess_edge_list(full_path)
        # save_to_file_with_prefix(output_dir, dataset_name, "preprocessed_edges.txt", preprocessed_edges)

        total_triangles = calculate_triangles(preprocessed_edges)
        print(f"Total number of triangles : {total_triangles}")

        print("vertex number: ", np.max(preprocessed_edges) + 1)

        print("Processing graph with virtual nodes ... ")
        duplicated_edges, virtual_node_mapping, virtual_node_counts, node_mapping = process_graph_with_virtual_nodes(
            preprocessed_edges, max_length
        )
        save_to_file_with_prefix(output_dir, dataset_name, "edgelist.txt", duplicated_edges)
        ## save_to_file_with_prefix(output_dir, dataset_name, "virtual_node_mapping.txt", virtual_node_mapping)
        ## save_to_file_with_prefix(output_dir, dataset_name, "virtual_node_counts.txt", virtual_node_counts)

        num_vertices = np.max(preprocessed_edges) + 1
        row, col = edgelist_to_csr(preprocessed_edges, num_vertices)
        # save_to_file_with_prefix(output_dir, dataset_name, "ori_row.txt", row)
        # save_to_file_with_prefix(output_dir, dataset_name, "ori_col.txt", col)

        print("Processing CSR with virtual nodes ... ")
        new_row, new_col, new_node_mapping, row_2 = process_csr_with_virtual_nodes(row, col, max_length)
        ## save_to_file_with_prefix(output_dir, dataset_name, "csr_row.txt", new_row)
        save_to_file_with_prefix(output_dir, dataset_name, "csr_col.txt", new_col)
        save_to_file_with_prefix(output_dir, dataset_name, "csr_row_2.txt", row_2)
        ## save_to_file_with_prefix(output_dir, dataset_name, "node_mapping.txt", new_node_mapping) ## same as virtual_node_mapping

        triangle_count = count_triangles_from_csr(new_row, new_col, duplicated_edges)
        print(f"Our method: Total number of triangles : {triangle_count}")

# Example usage
if __name__ == "__main__":
    max_length = (2048 - 16) ## keep 16 to avoid the aligned edge length exceed 2048.
    file_list = [
        "as20000102.txt",
        "amazon0302.mtx",
        "amazon0601.mtx",
        "cit-Patents.txt",
        "roadNet-PA.txt",
        "ca-cit-HepPh.edges",
        "facebook_combined.txt",
        "roadNet-CA.txt",
        "roadNet-TX.txt",
        "soc-Slashdot0811.txt",
    ]
    process_multiple_datasets(file_list, max_length, "../../datasets", "./generated")

