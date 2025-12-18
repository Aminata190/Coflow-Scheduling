import os
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import pandas as pd
import numpy as np
from tabulate import tabulate
import glob
from pathlib import Path
from IPython.display import display
from PIL import Image


# 
############################################################
#                                                          #
#               Extraction of data                         #
#                                                          #
############################################################
def extract_data(filename, row):
    with open(filename, 'r') as f:
       lines = f.readlines()
    
    data = []
    # print("Len data extract " + str(len(lines)))
    for line in lines:  
        data.append(float(line.strip().split("\t") [int(row)]))
    return data

    
def ratio_data(opt_data, cl_data):
    ratio = []
    for i in range(0,len(opt_data)) :
        ratio.append(cl_data[i]/opt_data[i])

    return ratio


# Extraction of Predictions
def extract_data_pred(filename, opt_data,n_delta=0, numero_inst= 0, all=0):
    with open(filename, 'r') as f:
       lines = f.readlines()
    
    k = 0
    meanValgen=[]
    minValgen=[]
    maxValgen=[]
    d= 10 # number of predictions errors delta={0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.99}
    if n_delta !=0: d= n_delta 
    for i in range (0,len(lines),d):    
        l = lines[i:i + d]  
        delta = [float(line.split("\t")[1]) for line in l]
        #Sin_Pred/OPT
        meanVal = [float(line.split("\t")[2])/float(opt_data[k]) for line in l]  
        # print(meanVal)
        minVal = [float(line.split("\t")[3])/float(opt_data[k]) for line in l]
        maxVal = [float(line.split("\t")[4])/float(opt_data[k]) for line in l]
        
        delta = np.array(delta)
        meanValgen.append(meanVal)
        minValgen.append(minVal)
        maxValgen.append(maxVal)
        k = k + 1
    minn=[]
    meann=[]
    maxn=[]
    min_f=[] 
    max_f=[]
    mean_f=[]

    nb_inst = int(len(lines) / delta.shape[0])
    # print("nb_inst: " + str(nb_inst))
    numero_inst =  int(numero_inst)
    for i in range (0,d,1):
        if (numero_inst != 0):
            minn.append(minValgen[numero_inst][i])
            maxn.append(maxValgen[numero_inst][i])
            meann.append(meanValgen[numero_inst][i])
        else:
            for j in range(0,nb_inst,1):
                minn.append(minValgen[j][i])
                maxn.append(maxValgen[j][i])
                # print(meanValgen[j][i])
                meann.append(meanValgen[j][i])
        
        # if all==1:min_f.append(sum(minn)/len(minn))
        # else: min_f.append(min(minn)) 
        min_f.append(min(minn))      
        max_f.append(max(maxn))  
        mean_f.append(sum(meann)/len(meann))
        # print("mean frequence: " +str(mean_f))

        minn.clear()
        maxn.clear()
        meann.clear()

    meanValue=np.array(mean_f)
    maxValue=np.array(max_f)
    minValue=np.array(min_f)
   
    return delta, meanValue, minValue, maxValue


def concatenation_pred_files(folder_path, output_file, prefix="results"):
    files = sorted(Path(folder_path).glob(f"{prefix}*"))  # e.g., results-*.txt
    if not files:
        raise FileNotFoundError(f"No files starting with {prefix} in {folder_path}")

    with open(output_file, "w", encoding="utf-8") as out:
        for i, path in enumerate(files):
            print(path)
            with open(path, "r", encoding="utf-8") as inp:
                out.write(inp.read())

def data_concatenation():
    folder_path = "/tmp/pfcalcul/Code_online_update/Outputs/paper/0.45/N100_L40/Pred"

    output_file = "/home/asangho/Desktop/These/coflow-scheduling/Prediction_Coflow_Scheduling/Code/Plots/Numerical/Online/Data/0.45/N100_L40/Pred/results-0-100_cct_pred_n=1000.txt"
    concatenation_pred_files(folder_path,output_file, prefix="results")


    output_file_SP_update = "/home/asangho/Desktop/These/coflow-scheduling/Prediction_Coflow_Scheduling/Code/Plots/Numerical/Online/Data/0.45/N100_L40/Pred/A_results-0-100_cct_pred_n=1000.txt"
    concatenation_pred_files(folder_path,output_file=output_file_SP_update, prefix="SpUpdate_results")


############################################################
#                                                          #
#               Extraction of quartiles                    #
#                                                          #
############################################################

def extract_data_pred_quartiles(filename, opt_data,n_delta=0, numero_inst= 0):
    with open(filename, 'r') as f:
       lines = f.readlines()
    
    k = 0
    difValgen=[]
    d= 10
    if n_delta !=0: d= n_delta
    # print("len pred "+ str(len(lines)))
    for i in range (0,len(lines),d):    
        # Extract the desired lines
        l = lines[i:i + d]  
        delta = [float(line.split("\t")[1]) for line in l]
        #Sin_Pred/OPT or Sin_Pred/CL
        difVal = [float(line.split("\t")[2])/float(opt_data[k]) for line in l] 
        delta = np.array(delta)
        difValgen.append(difVal)
        k = k + 1
    
  
    Q1 = []
    Q3 = []
    Q2 = []
    Mean = []
    Std = []
    nb_inst = int(len(lines) / delta.shape[0])
    
    numero_inst =  int(numero_inst)
    for i in range (d):
        difVal = []
        if (numero_inst != 0):
            difVal.append(difValgen[numero_inst][i])
        else:
            for j in range(nb_inst):
                difVal.append(difValgen[j][i])
                
        mean = np.mean(difVal)
        std= np.std(difVal)
        q1,q2,q3 = np.quantile(sorted(difVal), [0.25,0.5,0.75])
        

        Mean.append(round(mean, 3))
        Std.append(round(std, 3))
        Q1.append(round(q1, 3))
        Q2.append(round(q2, 3))
        Q3.append(round(q3, 3))

    return delta, Mean, Std, Q1,Q2,Q3

def quartiles_tables(filename, filename_compare,row = 1,numero_inst= 0, title="Pred/Opt", output_file=None):

    compare_data = extract_data(filename=filename_compare, row=row)

    _, Mean, Std, Q1,Q2,Q3 = extract_data_pred_quartiles(filename, compare_data,numero_inst= numero_inst)
    
    data = {
        # 'delta': delta,  
        'Mean': Mean,
        'Std' : Std, 
        'Q1': Q1,
        # 'Q2': Q2, 
        'Q3': Q3
    }

    df = pd.DataFrame(data)
    print("Quartiles Table for ratio of " + title )
    display(df)
    if output_file:
        df.to_latex(output_file, index=False, float_format="%.3f")
        print("sucess")


# combine_latex_tabulars.py

def extract_data_rows(file_path):
    rows = []
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            # Skip LaTeX formatting lines
            if line.startswith("\\") or line.startswith("Mean") or not line:
                continue
            # Remove trailing \\
            line = line.rstrip("\\")
            rows.append(line)
    return rows

def combination_2_files(f1,f2,output_f):

    f1_rows = extract_data_rows(f1)
    f2_rows = extract_data_rows(f2)

    
    delta_values = [0.00, 0.1,0.20,0.30,0.40,0.50,0.60,0.70,0.80,0.99]

    # Combine all rows
    combined = []
    for i in range(len(delta_values)):
        line = delta_values[i]
        f1_line = f1_rows[i]
        f2_line = f2_rows[i]
        combined.append(f"{line:.2f} & {f1_line} & {f2_line} \\\\")

    # Save to output file
    with open(output_f, "w") as f_out:
        f_out.write("\n".join(combined))

    print("✅ Combined table saved to "+ output_f)


############################################################
#                                                          #
#               Line plots                                 #
#                                                          #
############################################################

def plot_all(delta, meanVal, minVal, maxVal,list_configuration,nRow=None, nCol=None, numero_inst=1,y_bottom=0.99,ytitle=r'$\frac{C{pred}}{C_OPT}$',output_file=None) :
    plt.rcParams['text.usetex'] = True
    plt.rcParams.update({'font.size': 15})
    
    nb_plots = len(meanVal)
    # print(nb_plots)

    # Déduction automatique du nombre de lignes/colonnes
    if nb_plots == 1:
        nRow, nCol = 1, 1
    elif nb_plots % 2 == 0:
        nRow, nCol = nb_plots // 2, 2
    else:
        nRow, nCol = nb_plots // 2 + 1, 2

    if   nb_plots == 2:
        fig, axes = plt.subplots(nrows=nRow, ncols=nCol, figsize=(8, 4))
    else:fig, axes = plt.subplots(nrows=nRow, ncols=nCol, figsize=(8, 8))
    axes = axes.flatten() if isinstance(axes, (list, np.ndarray)) or (nRow > 1 or nCol > 1) else [axes]

    for i in range(nb_plots):
        ax = axes[i]
        ax.plot(delta, minVal[i], color='#0BE340', marker='*', linestyle='None', label='Min ratio')
        ax.plot(delta, maxVal[i], 'bo', label='Max ratio')
        ax.fill_between(delta, minVal[i], maxVal[i], alpha=0.2, color='blue')
        ax.plot(delta, meanVal[i], color='#E30B15', label='Mean ratio')

        ax.set_xlabel(r'$\delta$', fontsize=14)
        # ax.set_ylabel(ytitle, fontsize=14)
        ax.set_title(list_configuration[i], fontsize=12)
        ax.legend(fontsize=10)
    

    # En cas de figure vide (ex: axes inutilisés)
    for j in range(nb_plots, len(axes)):
        fig.delaxes(axes[j])

    plt.tight_layout(rect=[0, 0, 1, 0.92])
    plt.ylim(y_bottom, None)


    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved plot to {output_file}")

    plt.show()


def line_plot_all(list_filename, list_filename_compare,list_configuration,row = 1,numero_inst= 0,all=0,y_bottom=1.99,output_file=None,ytitle=r'$\frac{C{pred}}{C_OPT}$'):
    list_compare_data = [];list_maxVal = [];list_minVal = [];list_meanVal = []

    for i in range(len(list_configuration)):
        list_compare_data.append(extract_data(filename=list_filename_compare[i], row=row))
        delta, meanVal, minVal, maxVal = extract_data_pred(list_filename[i],list_compare_data[i],numero_inst=numero_inst,all=all)
        list_maxVal.append(maxVal)
        list_minVal.append(minVal)
        list_meanVal.append(meanVal)

    # print(len(list_minVal))
    plot_all(delta, meanVal=list_meanVal, minVal=list_minVal, maxVal=list_maxVal, numero_inst=numero_inst,ytitle=ytitle,y_bottom=y_bottom,list_configuration=list_configuration,output_file=output_file)


def extract_pred_ratio(list_filename_pred, list_filename_cl,
                       list_configuration, row=1, numero_inst=0, all=0):
    list_minVal, list_meanVal, list_maxVal = [], [], []

    for i in range(len(list_configuration)):
        cl_data = extract_data(filename=list_filename_cl[i], row=row)
        delta, meanVal, minVal, maxVal = extract_data_pred(
            list_filename_pred[i],
            cl_data,
            numero_inst=numero_inst,
            all=all
        )
        list_minVal.append(minVal)
        list_meanVal.append(meanVal)
        list_maxVal.append(maxVal)

    return delta, list_minVal, list_meanVal, list_maxVal

def plot_sp_vs_asp(delta,
                   sp_min, sp_mean, sp_max,
                   asp_min, asp_mean, asp_max,
                   list_configuration,
                   y_bottom=1.0,
                   ytitle=r'$\frac{C_{\mathrm{Pred}}}{C_{\mathrm{CL}}}$',
                   output_file=None):

    plt.rcParams['text.usetex'] = True
    plt.rcParams.update({'font.size': 14})

    nb_plots = len(list_configuration)

    nRow = nb_plots // 2 + nb_plots % 2
    nCol = 2 if nb_plots > 1 else 1

    fig, axes = plt.subplots(nRow, nCol, figsize=(8, 4*nRow))
    axes = axes.flatten() if nb_plots > 1 else [axes]

    for i in range(nb_plots):
        ax = axes[i]

        # ===== SP classique (ROUGE) =====
        ax.fill_between(delta, sp_min[i], sp_max[i],
                        color='red', alpha=0.15)
        ax.plot(delta, sp_mean[i], color='red',
                label='SP (mean)', linewidth=2)

        # ===== Anticipate-SP (VERT) =====
        ax.fill_between(delta, asp_min[i], asp_max[i],
                        color='blue', alpha=0.15)
        ax.plot(delta, asp_mean[i], color='blue',
                linestyle='--', label='Anticipate-SP (mean)', linewidth=2)

        ax.set_title(list_configuration[i])
        ax.set_xlabel(r'$\delta$')
        ax.set_ylim(y_bottom, None)
        ax.legend(fontsize=10)

    for j in range(nb_plots, len(axes)):
        fig.delaxes(axes[j])

    # fig.supylabel(ytitle)
    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved plot to {output_file}")

    plt.show()


def line_plot_combined(
    list_filename,
    list_filename_a,
    list_filename_compare,
    list_configuration,
    row=3,numero_inst= 0,all=0,y_bottom=1.99,output_file=None):
   # ===== SP =====
    delta, sp_min, sp_mean, sp_max = extract_pred_ratio(
        list_filename,
        list_filename_compare,
        list_configuration,
        row,numero_inst,all)

    # ===== Anticipate-SP =====
    _, asp_min, asp_mean, asp_max = extract_pred_ratio(
        list_filename_a,
        list_filename_compare,
        list_configuration,
        row,numero_inst,all)

    plot_sp_vs_asp(
        delta,
        sp_min, sp_mean, sp_max,
        asp_min, asp_mean, asp_max,
        list_configuration,
        y_bottom,
        output_file=output_file
    ) 

############################################################
#                                                          #
#               Stacked Bars                               #
#                                                          #
############################################################

def bars_stacked(list_filename, list_filename_compare, list_configuration, 
                     row=1, numero_inst=0,y_bottom=0, output_file=None):

    list_compare_data = []
    list_meanVal = []
    list_delta_labels = None

    for i in range(len(list_configuration)):
        opt_values = extract_data(filename=list_filename_compare[i], row=row)
        delta, meanVal, minVal, maxVal = extract_data_pred(list_filename[i], opt_values, numero_inst=numero_inst)

        if list_delta_labels is None:
            print(delta)
            list_delta_labels = [r'$\delta$ = ' +str(d) for d in delta]

        list_compare_data.append(opt_values)
        list_meanVal.append(meanVal)

    n_config = len(list_configuration)
    n_delta = len(list_delta_labels)

    x = np.arange(n_config)

    cmap = cm.get_cmap('jet', n_delta)
    colors = [cmap(i) for i in range(n_delta)]

    fig, ax = plt.subplots(figsize=(12, 6))

    # Initialiser la base (bottom) à zéro pour chaque barre
    bottoms = np.zeros(n_config)
    list_delta_labels
    for i in range(n_delta-1, -1, -1):
        y_values = [list_meanVal[d][i] for d in range(n_config)]
        ax.bar(x, y_values, bottom=bottoms, width=0.6, color=colors[i], label=list_delta_labels[i])


    ax.set_xlabel('Configurations', fontsize=14)
    # ax.set_ylabel(ytitle, fontsize=14)
    if y_bottom!=0: ax.set_ylim(bottom=y_bottom)
    # ax.set_title("Impact of Prediction Error on Different Topologies", fontsize=16)
    ax.set_xticks(x)
    ax.set_xticklabels(list_configuration, rotation=45)
    ax.legend(title="Prediction errors", fontsize=9)
    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved histogram to {output_file}")

    plt.show()


############################################################
#                                                          #
#               Histogramm                                 #
#                                                          #
############################################################
## For Predictions
def extraction(filename, opt_data,n_delta = 0, numero_inst= 0):
    with open(filename, 'r') as f:
       lines = f.readlines()
    
    k = 0
    meanValgen=[]
    d = 10
    if n_delta !=0: d=n_delta
    for i in range (0,len(lines),d):    
        # Extract the desired lines
        l = lines[i:i + d]  
        delta = [float(line.split("\t")[1]) for line in l]
        #Sin_Pred/OPT
        meanVal = [float(line.split("\t")[2])/float(opt_data[k]) for line in l]  
        delta = np.array(delta)
        meanValgen.append(meanVal)
        k = k + 1
    return delta,meanValgen


def extract_data_pred_histo_cumul(list_filename, list_filename_compare,
                     row=1, numero_inst=0):
    
    list_compare_data = []
    list_meanVal = []
    list_delta_labels = None

    # for i in range(len(list_configuration)): true value
    for i in range(len(list_filename)):  # not true
        opt_values = extract_data(filename=list_filename_compare[i], row=row)
        delta, meanVal = extraction(list_filename[i], opt_values, numero_inst=numero_inst)

        if list_delta_labels is None:
            list_delta_labels = [r'$\delta$ = ' +str(d) for d in delta]

        list_compare_data.append(opt_values)
        list_meanVal.append(meanVal)

    return list_delta_labels, list_meanVal


def plot_hist_cumul(list_delta_labels,meanVal,conf,reduit=False, output_file= None) :
    ind_delta = [0,5,9]
    if reduit: 
        fig, axes = plt.subplots(nrows=1, ncols= 3, figsize=(12, 4))
        n_delta = len(ind_delta)
    else:   
        fig, axes = plt.subplots(nrows=2, ncols= 5, figsize=(12, 4))
        n_delta = len(list_delta_labels)
    axes = axes.flatten()

    cmap = cm.get_cmap('jet', n_delta)
    colors = [cmap(i) for i in range(n_delta)]

    # plt.suptitle(conf, fontsize=14)

    for i in range(n_delta): 
        if reduit:
            data = [meanVal[c][ind_delta[i]] for c in range(len(meanVal))]
            axes[i].hist(data, bins=15, edgecolor='black', color=colors[i])
            axes[i].set_title(list_delta_labels[ind_delta[i]], fontsize=18)
            axes[i].tick_params(axis='both', which='major', labelsize=18) 
        else: 
            data = [meanVal[c][i] for c in range(len(meanVal))]
            axes[i].hist(data, bins=15, edgecolor='black', color=colors[i])
            axes[i].set_title(list_delta_labels[i], fontsize=18)
            axes[i].tick_params(axis='both', which='major', labelsize=18)

    plt.tight_layout(rect=[0, 0, 1, 0.95])  # Leaves space for the suptitle
    # plt.xlim(0.95, None)
    
    plt.tight_layout()
    # plt.title(conf)

    # Save the figure if output_file is given
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved histogram to {output_file}")


def histogramm_cumulation(list_filename, list_filename_compare, list_configuration, 
                     row=1, numero_inst=0,reduit=False, output_file=None):

    list_delta_labels,list_meanVal = extract_data_pred_histo_cumul(list_filename, list_filename_compare,
                     row=row, numero_inst=numero_inst)
    
    for id_conf in range(len(list_configuration)):
        # print("Configuration: " + str(list_configuration[id_conf]))
        if output_file: output_file_conf = output_file + str(id_conf)+ ".png"
        else: output_file_conf = None
        plot_hist_cumul(list_delta_labels=list_delta_labels,meanVal= list_meanVal[id_conf],conf=list_configuration[id_conf],reduit=reduit, output_file=output_file_conf )




############################################################
#                                                          #
#               Bars Graphs:                               #
#                                                          #
############################################################

def extraction_for_bars(filename, off_data,n_delta=0):
    with open(filename, 'r') as f:
       lines = f.readlines()

    k = 0
    d= 10 # number of predictions errors delta={0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.99}
    if n_delta !=0: d= n_delta 

    meanVal = []
    for i in range (0,len(lines),d):    
        meanVal.append(float(lines[i].split("\t")[2])/float(off_data[k]))
        k = k+1

    return meanVal



### Min,Mean, Max values of ratio CL(online)/CL(offline) for differents configuration###

def bar_graph_extend(list_filename_on, list_filename_off, list_configuration, numero_inst=0, y_slim=0.99, output_file=None):
    list_ratio_data = []  # Chaque élément sera une liste : [min, mean, max]

    for i in range(len(list_configuration)):
        off_values = extract_data(filename=list_filename_off[i], row=3)
        values = extraction_for_bars(filename=list_filename_on[i], off_data=off_values)
        # print(off_values)
        # print("--------------------")
        # print(on_values)

        # ratio_ = ratio_data(opt_data=on_values, cl_data=off_values)

        ratio_min = min(values)
        ratio_mean = sum(values) / len(values)
        ratio_max = max(values)

        list_ratio_data.append([ratio_min, ratio_mean, ratio_max])

    list_oper = ["min", "mean", "max"]
    n_config = len(list_configuration)
    n_oper = len(list_oper)

    bar_width = 0.7 / n_oper
    x = np.arange(n_config)

    cmap = cm.get_cmap('jet', n_oper) #jet, gist_ncar,nipy_spectral,hsv
    colors = [cmap(i) for i in range(n_oper)]
    # colors = ["green", "yellow", "red"]

    fig, ax = plt.subplots(figsize=(12, 5))

    for i in range(n_oper):
        offsets = x + i * bar_width - (bar_width * (n_oper - 1) / 2)
        y_values = [list_ratio_data[d][i] for d in range(n_config)]
        bars = ax.bar(offsets, y_values, width=bar_width, color=colors[i], label=list_oper[i])

        # Ajouter les valeurs au-dessus des barres
        for j, val in enumerate(y_values):
            ax.text(offsets[j], val + 0.01, f"{val:.2f}", ha='center', va='bottom', fontsize=8)

    ax.set_xlabel('Configurations', fontsize=14)
    ax.set_ylim(bottom=y_slim)
    # ax.set_title("Ratio C_cl / C_opt for each configuration", fontsize=16)
    ax.set_xticks(x)
    ax.set_xticklabels(list_configuration, rotation=45)
    ax.legend(title="Metric", fontsize=9)

    plt.tight_layout()
    # Save the figure if output_file is specified
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved bars graphs to {output_file}")

    plt.show()


### Mean values of ratio C_pred/alg for differents###

def bar_graph_pred(list_filename, list_filename_compare, list_configuration, 
                     row=1, numero_inst=0,y_slim=0.99, output_file=None):

    list_compare_data = []
    list_meanVal = []
    list_delta_labels = None

    for i in range(len(list_configuration)):
        opt_values = extract_data(filename=list_filename_compare[i], row=row)
        delta, meanVal, minVal, maxVal = extract_data_pred(list_filename[i], opt_values, numero_inst=numero_inst)

        if list_delta_labels is None:
            list_delta_labels = [r'$\delta$ = ' +str(d) for d in (delta)]  # étiquettes fictives

        list_compare_data.append(opt_values)
        list_meanVal.append(meanVal)

    n_config = len(list_configuration)
    n_delta = len(list_delta_labels)

    bar_width = 0.7 / n_delta
    x = np.arange(n_config)

    cmap = cm.get_cmap('jet', n_delta)   #jet, gist_ncar,nipy_spectral,hsv
    colors = [cmap(i) for i in range(n_delta)]

    # fig, ax = plt.subplots(figsize=(10, 3 + n_delta))
    fig, ax = plt.subplots(figsize=(12, n_delta -4))

    for i in range(n_delta):
        offsets = x + i * bar_width - (bar_width * (n_delta - 1) / 2)
        y_values = [list_meanVal[d][i] for d in range(n_config)]
        ax.bar(offsets, y_values, width=bar_width, color=colors[i % len(colors)], label=list_delta_labels[i])

    ax.set_xlabel(r'Configurations', fontsize=14)
    # ax.set_ylabel(ytitle, fontsize=14)
    ax.set_ylim(bottom=y_slim)
    # ax.set_title("Impact of Prediction Error on Different Topologies", fontsize=16)
    ax.set_xticks(x)
    ax.set_xticklabels(list_configuration, rotation=45)
    ax.legend(title="Prediction errors", fontsize=9)
    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved histogram to {output_file}")

    plt.show()



############################################################
#                                                          #
#               COnversion .png to .pdf                    #
#                                                          #
############################################################

# image_path: image without .png
def image_to_pdf(image_path):    
    img = Image.open(image_path+".png").convert("RGB") 
    img.save(image_path+".pdf")

def folders_png_pdf(png_folder):
    for img in png_folder:
        # print(img)
        image_to_pdf(img)

############################################################
#                                                          #
#               Extraction of quartiles                    #
#                                                          #
############################################################