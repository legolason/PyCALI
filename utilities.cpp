
#include <iostream>
#include <fstream> 
#include <sstream>
#include <string>
#include <algorithm>
#include <numeric>
#include <cblas.h>
#include <float.h>

#include "utilities.hpp"
#include "mathfun.h"

/*=====================================================*/
Data::Data()
{
}

Data::Data(const string& fname)
{
  load(fname);
}

Data::~Data()
{
  flux_org.clear();
  error_org.clear();
  time.clear();
  flux.clear();
  error.clear();
  code.clear();

  num_code.clear();
  code_list.clear();
}

void Data::load(const string& fname)
{
  fstream fin;
  string line;
  stringstream ss;

  fin.open(fname);
  if(fin.fail())
  {
    printf("cannot open file %s\n", fname);
    exit(-1);
  }

  int i, j, num;
  int idx, idx_str;
  double t, f, e;
  string cstr;
  
  idx = 0;
  while(1)
  {
    getline(fin, line);
    if(fin.fail())
      break;

    line.erase(0, 1);  /* skip the first charactor '#' */
    idx_str = line.find_first_not_of(WhiteSpace);
    line.erase(0, idx_str);  /* skip the left white spaces */

    idx_str = line.find_first_of(WhiteSpace); /* extract the code string */
    cstr = line.substr(0, idx_str);
    code_list.push_back(cstr);

    line.erase(0, idx_str); /* extract the number */
    num = stoi(line);
    num_code.push_back(num);
    cout<<code_list[idx]<<"   "<<num_code[idx]<<endl;
    for(j=0; j<num_code[idx]; j++)
    {
      getline(fin, line);
      if(fin.fail())
      {
        cout<<"# Wrong in reading "<<fname<<endl;
        exit(-1);
      }

      ss.str(line);
      ss>>t>>f>>e;
      if(ss.fail())
      {
        cout<<"# Wrong in reading "<<fname<<endl;
        exit(-1);
      }
        
      time.push_back(t);
      flux_org.push_back(f);
      error_org.push_back(e);
      code.push_back(idx);
      ss.clear();
    }
    idx++;
  }
  cout<<time.size()<<" points, "<<code_list.size()<<" codes."<<endl;
  fin.close();

  normalize();
  sort_data();
  return;
}

void Data::normalize()
{
  int i;
  norm = 0.0;
  for(i=0; i<flux_org.size(); i++)
  {
    norm += flux_org[i];
  }
  norm /= flux_org.size();
  for(i=0; i<flux_org.size(); i++)
  {
    flux_org[i] /= norm;
    error_org[i] /= norm;
  }

  return;
}

void Data::sort_data()
{
  vector<int> code_tmp;
  vector<double> time_tmp;

  index.resize(time.size());
  iota(index.begin(), index.end(), 0);
  stable_sort(index.begin(), index.end(), [&](size_t i1, size_t i2) {return time[i1] < time[i2];});
  
  time_tmp = time;
  flux = flux_org;
  error = error_org;
  code_tmp = code;
  int i;
  for(i=0; i<index.size(); i++)
  {
    time[i] = time_tmp[index[i]];
    flux_org[i] = flux[index[i]];
    error_org[i] = error[index[i]];
    code[i] = code_tmp[index[i]];
  }
  code_tmp.clear();
  time_tmp.clear();
}

/*=====================================================*/
/* class for calibration */
Cali *cali;

Cali::Cali()
{
  num_params = 0;
  par_range_model = NULL;
  par_fix = NULL;
  par_fix_val = NULL;

  par_prior_model = NULL;
  par_prior_gaussian = NULL;

  best_params = NULL;
  best_params_std = NULL;
  best_params_covar = NULL;

  workspace = NULL;

  dnest_free_fptrset(fptrset);
}

Cali::Cali(const string& fcont, const string& fline)
     :cont(fcont)
{
  int i, j;

  num_params_var = 2;
  size_max = cont.time.size();
  ncode = cont.code_list.size();
  if(!fline.empty())
  {
    line.load(fline);
    size_max = fmax(size_max, line.time.size());
    ncode = fmax(ncode, line.code_list.size());
    num_params_var += 2;
  }
  num_params = num_params_var + ncode*2;
  par_range_model = new double * [num_params];
  for(i=0; i<num_params; i++)
  {
    par_range_model[i] = new double [2];
  }
  par_fix = new bool [num_params];
  par_fix_val = new double [num_params];
  par_prior_model = new int [num_params];
  par_prior_gaussian = new double * [num_params];
  for(i=0; i<num_params; i++)
  {
    par_prior_gaussian[i] = new double [2];
  } 
  best_params = new double[num_params];
  best_params_std = new double[num_params];
  best_params_covar = new double[num_params*num_params];

  /* set parameter ranges */
  i=0; /* sigma */
  par_range_model[i][0] = log(1.0e-6);
  par_range_model[i][1] = log(1.0);
  par_prior_model[i] = UNIFORM;
  par_prior_gaussian[i][0] = 0.0;
  par_prior_gaussian[i][1] = 0.0;
  
  i+=1; /* tau */
  par_range_model[i][0] = log(1.0);
  par_range_model[i][1] = log(1.0e4);
  par_prior_model[i] = UNIFORM;
  par_prior_gaussian[i][0] = 0.0;
  par_prior_gaussian[i][1] = 0.0;

  if(!fline.empty())
  {
    i+=1; /* sigma */
    par_range_model[i][0] = log(1.0e-6);
    par_range_model[i][1] = log(1.0);
    par_prior_model[i] = UNIFORM;
    par_prior_gaussian[i][0] = 0.0;
    par_prior_gaussian[i][1] = 0.0;
  
    i+=1; /* tau */
    par_range_model[i][0] = log(1.0);
    par_range_model[i][1] = log(1.0e4);
    par_prior_model[i] = UNIFORM;
    par_prior_gaussian[i][0] = 0.0;
    par_prior_gaussian[i][1] = 0.0;
  }
  
  for(j=0; j<ncode; j++)
  {
    i+=1;
    par_range_model[i][0] = 0.5;
    par_range_model[i][1] = 1.5;
  }
  for(j=0; j<ncode; j++)
  {
    i+=1;
    par_range_model[i][0] = -1.0;
    par_range_model[i][1] = 1.0;
  }

  for(i=0; i<num_params; i++)
  {
    par_fix[i] = NOFIXED;
    par_fix_val[i] = -DBL_MAX;
  }
  par_fix[num_params_var] = FIXED;
  par_fix_val[num_params_var] = 1.0;
  par_fix[num_params_var+ncode] = FIXED;
  par_fix_val[num_params_var+ncode] = 0.0;

  workspace = new double[10*size_max];
  
  fptrset = dnest_malloc_fptrset();
  fptrset->from_prior = from_prior_cali;
  fptrset->perturb = perturb_cali;
  fptrset->print_particle = print_particle_cali;
  fptrset->log_likelihoods_cal = prob_cali;
}

Cali::~Cali()
{
  int i;

  delete[] par_fix;
  delete[] par_fix_val;
  delete[] par_prior_model;
  for(i=0; i<num_params; i++)
  {
    delete[] par_range_model[i];
    delete[] par_prior_gaussian[i];
  }
  delete[] par_range_model;
  delete[] par_prior_gaussian;
  delete[] best_params;
  delete[] best_params_std;
  delete[] best_params_covar;

  delete[] workspace;
}

void Cali::align(double *model)
{
  int i, idx;
  double *ps_scale = model+num_params_var;
  double *es_shift = model+num_params_var + ncode;
  for(i=0; i<cont.time.size(); i++)
  {
    idx = cont.code[i];
    cont.flux[i] = cont.flux_org[i] * ps_scale[idx] - es_shift[idx];
    cont.error[i] = cont.error_org[i] * ps_scale[idx];
  }

  if(line.time.size()>0)
  {
    for(i=0; i<line.time.size(); i++)
    {
      idx = line.code[i];
      line.flux[i] = line.flux_org[i] * ps_scale[idx];
      line.error[i] = line.error_org[i] * ps_scale[idx];
    }
  }
}

void Cali::align_with_error()
{
  int i, idx;
  double *ps_scale = best_params+num_params_var;
  double *es_shift = best_params+num_params_var + ncode;
  double *ps_scale_err = best_params_std + num_params_var;
  double *es_shift_err = best_params_std + num_params_var + ncode;
  for(i=0; i<cont.time.size(); i++)
  {
    idx = cont.code[i];
    cont.flux[i] = cont.flux_org[i] * ps_scale[idx] - es_shift[idx];
    cont.error[i] = sqrt(pow(cont.error_org[i] * ps_scale[idx], 2.0)
                        +pow(cont.flux_org[i]*ps_scale_err[idx], 2.0)
                        +pow(es_shift_err[idx], 2.0)
                        -2.0*cont.flux_org[i]*best_params_covar[(num_params_var+idx)*num_params + (num_params_var+idx+ncode)]
                        );
  }

  if(line.time.size()>0)
  {
    for(i=0; i<line.time.size(); i++)
    {
      idx = line.code[i];
      line.flux[i] = line.flux_org[i] * ps_scale[idx];
      line.error[i] = sqrt(pow(line.error_org[i] * ps_scale[idx], 2.0)
                          +pow(line.flux_org[i] * ps_scale_err[idx], 2.0)
                          );
    }
  }
}

void Cali::mcmc()
{
  int i, argc=0;
  char **argv;
  double logz_con;
  char dnest_options_file[256];

  argv = new char * [9];
  for(i=0; i<9; i++)
  {
    argv[i] = new char [256];
  }
  
  strcpy(argv[argc++], "dnest");
  strcpy(argv[argc++], "-s");
  strcpy(argv[argc], "./");
  strcat(argv[argc++], "/data/restart_dnest.txt");

  strcpy(dnest_options_file, "OPTIONS");
  logz_con = dnest(argc, argv, fptrset, num_params, "data/", dnest_options_file);

  for(i=0; i<9; i++)
  {
    delete[] argv[i];
  }
  delete[] argv;
}

void Cali::get_best_params()
{
  int i, j, num_ps;
  FILE *fp;
  char posterior_sample_file[256];
  double *post_model, *posterior_sample;
  double *pm, *pmstd;

  strcpy(posterior_sample_file, "data/posterior_sample.txt");

  /* open file for posterior sample */
  fp = fopen(posterior_sample_file, "r");
  if(fp == NULL)
  {
    fprintf(stderr, "# Error: Cannot open file %s.\n", posterior_sample_file);
    exit(0);
  }

  /* read number of points in posterior sample */
  if(fscanf(fp, "# %d", &num_ps) < 1)
  {
    fprintf(stderr, "# Error: Cannot read file %s.\n", posterior_sample_file);
    exit(0);
  }
  printf("# Number of points in posterior sample: %d\n", num_ps);

  post_model = new double[num_params*sizeof(double)];
  posterior_sample = new double[num_ps * num_params*sizeof(double)];
  
  for(i=0; i<num_ps; i++)
  {
    for(j=0; j<num_params; j++)
    {
      if(fscanf(fp, "%lf", (double *)post_model + j) < 1)
      {
        fprintf(stderr, "# Error: Cannot read file %s.\n", posterior_sample_file);
        exit(0);
      }
    }
    fscanf(fp, "\n");

    memcpy(posterior_sample+i*num_params, post_model, num_params*sizeof(double));

  }
  fclose(fp);

  /* calcaulte mean and standard deviation of posterior samples. */
  pm = (double *)best_params;
  pmstd = (double *)best_params_std;
  for(j=0; j<num_params; j++)
  {
    pm[j] = pmstd[j] = 0.0;
  }
  for(i=0; i<num_ps; i++)
  {
    for(j =0; j<num_params; j++)
      pm[j] += *((double *)posterior_sample + i*num_params + j );
  }

  for(j=0; j<num_params; j++)
    pm[j] /= num_ps;

  for(i=0; i<num_ps; i++)
  {
    for(j=0; j<num_params; j++)
      pmstd[j] += pow( *((double *)posterior_sample + i*num_params + j ) - pm[j], 2.0 );
  }

  for(j=0; j<num_params; j++)
  {
    if(num_ps > 1)
      pmstd[j] = sqrt(pmstd[j]/(num_ps-1.0));
    else
      pmstd[j] = 0.0;
  }  

  for(j = 0; j<num_params; j++)
    printf("Best params %d %f +- %f\n", j, *((double *)best_params + j), 
                                           *((double *)best_params_std + j) ); 

  /* calculate covariance */
  double covar;
  int k;
  for(i=0; i<num_params; i++)
  {
    for(j=0; j<i; j++)
    {
      covar = 0.0;
      for(k=0; k<num_ps; k++)
      {
        covar += (*((double *)posterior_sample + k*num_params + i )) * (*((double *)posterior_sample + k*num_params + j ));
      }
      best_params_covar[i*num_params+j] = best_params_covar[j*num_params+i] = covar/num_ps - best_params[i]*best_params[j];
    }
    best_params_covar[i*num_params+i] = best_params_std[i] * best_params_std[i];
  }

  delete[] post_model;
  delete[] posterior_sample;
}
/*=============================================================*/
double prob_cali(const void *model)
{
  double prob, prob1=0.0, prob2=0.0, lambda, ave_con, lndet, sigma, sigma2, tau, alpha;
  double lndet_n, lndet_n0, prior_phi;
  double * ybuf, * Larr, *W, *D, *phi, *Cq, *Lbuf, *yq;
  int i, nq, info;
  double *workspace = cali->workspace;
  double *pm = (double *)model;
  double *ps_scale = pm + cali->num_params_var;
  Data &cont = cali->cont;
  int nd_cont = cont.time.size();

  nq = 1;
  Larr = workspace;
  Lbuf = Larr + nd_cont*nq;
  ybuf = Lbuf + nd_cont*nq;
  W = ybuf + nd_cont;
  D = W + nd_cont;
  phi = D + nd_cont;
  Cq = phi + nd_cont;
  yq = Cq + nq*nq;

  tau = pm[1];
  sigma = pm[0] * sqrt(tau);
  sigma2 = sigma*sigma;
  
  cali->align(pm);

  for(i=0;i<nd_cont;i++)
    Larr[i]=1.0;

  compute_semiseparable_drw(cont.time.data(), nd_cont, sigma2, 1.0/tau, cont.error.data(), 0.0, W, D, phi);
  lndet = 0.0;
  for(i=0; i<nd_cont; i++)
    lndet += log(D[i]);

  /* calculate L^T*C^-1*L */
  multiply_mat_semiseparable_drw(Larr, W, D, phi, nd_cont, nq, sigma2, Lbuf);
  multiply_mat_MN_transposeA(Larr, Lbuf, Cq, nq, nq, nd_cont);

  /* calculate L^T*C^-1*y */
  multiply_matvec_semiseparable_drw(cont.flux.data(), W, D, phi, nd_cont, sigma2, ybuf);
  multiply_mat_MN_transposeA(Larr, ybuf, yq, nq, 1, nd_cont);
  
  lambda = Cq[0];
  ave_con = yq[0]/Cq[0];

/* get the probability */
  for(i=0;i<nd_cont;i++)
  {
    ybuf[i] = cont.flux[i] - ave_con;
  }
  multiply_matvec_semiseparable_drw(ybuf, W, D, phi, nd_cont, sigma2, Lbuf);
  prob1 = -0.5 * cblas_ddot(nd_cont, ybuf, 1, Lbuf, 1);
  
  lndet_n = lndet_n0 = 0.0;
  for(i=0; i<nd_cont; i++)
  {
    lndet_n += 2.0*log(cont.error[i]);
    lndet_n0 += 2.0*log(cont.error_org[i]);
  }
  prob1 = prob1 - 0.5*lndet - 0.5*log(lambda) + 0.5 * (lndet_n - lndet_n0);
  
  if(cali->line.time.size() > 0)
  {
    Data& line = cali->line;
    int nd_line = line.time.size();

    Larr = workspace;
    Lbuf = Larr + nd_line*nq;
    ybuf = Lbuf + nd_line*nq;
    W = ybuf + nd_line;
    D = W + nd_line;
    phi = D + nd_line;
    Cq = phi + nd_line;
    yq = Cq + nq*nq;

    tau = pm[3];
    sigma = pm[2]*sqrt(tau);
    sigma2 = sigma*sigma;

    for(i=0;i<nd_line;i++)
      Larr[i]=1.0;

    compute_semiseparable_drw(line.time.data(), nd_line, sigma2, 1.0/tau, line.error.data(), 0.0, W, D, phi);
    lndet = 0.0;
    for(i=0; i<nd_line; i++)
      lndet += log(D[i]);

    /* calculate L^T*C^-1*L */
    multiply_mat_semiseparable_drw(Larr, W, D, phi, nd_line, nq, sigma2, Lbuf);
    multiply_mat_MN_transposeA(Larr, Lbuf, Cq, nq, nq, nd_line);

    /* calculate L^T*C^-1*y */
    multiply_matvec_semiseparable_drw(line.flux.data(), W, D, phi, nd_line, sigma2, ybuf);
    multiply_mat_MN_transposeA(Larr, ybuf, yq, nq, 1, nd_line);
  
    lambda = Cq[0];
    ave_con = yq[0]/Cq[0];

    for(i=0;i<nd_line;i++)
    {
      ybuf[i] = line.flux[i] - ave_con;
    }
    multiply_matvec_semiseparable_drw(ybuf, W, D, phi, nd_line, sigma2, Lbuf);
    prob2 = -0.5 * cblas_ddot(nd_line, ybuf, 1, Lbuf, 1);

    lndet_n = lndet_n0 = 0.0;
    for(i=0; i<nd_line; i++)
    {
      lndet_n += 2.0*log(line.error[i]);
      lndet_n0 += 2.0*log(line.error_org[i]);
    }

    prob2 = prob2 - 0.5*lndet - 0.5*log(lambda) + 0.5 * (lndet_n - lndet_n0);
  }
  
  prior_phi = 1.0;
  for(i=0; i<cali->ncode; i++)
  {
    prior_phi *= ps_scale[i]; 
  }
  
  prob = prob1 + prob2 - log(prior_phi);
  
  return prob;
}
void from_prior_cali(void *model)
{
  int i;
  double *pm = (double *)model;

  for(i=0; i<cali->num_params; i++)
  {
    if(cali->par_prior_model[i] == GAUSSIAN )
    {
      pm[i] = dnest_randn() * cali->par_prior_gaussian[i][1] + cali->par_prior_gaussian[i][0];
      dnest_wrap(&pm[i], cali->par_range_model[i][0], cali->par_range_model[i][1]);
    }
    else
    {
      pm[i] = cali->par_range_model[i][0] + dnest_rand()*(cali->par_range_model[i][1] - cali->par_range_model[i][0]);
    }
  }

  for(i=0; i<cali->num_params; i++)
  {
    if(cali->par_fix[i] == FIXED)
      pm[i] = cali->par_fix_val[i];
  }
}
void print_particle_cali(FILE *fp, const void *model)
{
  int i;
  double *pm = (double *)model;

  for(i=0; i<cali->num_params; i++)
  {
    fprintf(fp, "%e ", pm[i] );
  }
  fprintf(fp, "\n");
}
double perturb_cali(void *model)
{
  double *pm = (double *)model;
  double logH = 0.0, width;
  int which;
  
  /* sample variability parameters more frequently */
  do
  {
    which = dnest_rand_int(cali->num_params);
  }while(cali->par_fix[which] == FIXED);
 
  width = ( cali->par_range_model[which][1] - cali->par_range_model[which][0] );

  if(cali->par_prior_model[which] == GAUSSIAN)
  {
    logH -= (-0.5*pow((pm[which] - cali->par_prior_gaussian[which][0])/cali->par_prior_gaussian[which][1], 2.0) );
    pm[which] += dnest_randh() * width;
    dnest_wrap(&pm[which], cali->par_range_model[which][0], cali->par_range_model[which][1]);
    logH += (-0.5*pow((pm[which] - cali->par_prior_gaussian[which][0])/cali->par_prior_gaussian[which][1], 2.0) );
  }
  else
  {
    pm[which] += dnest_randh() * width;
    dnest_wrap(&(pm[which]), cali->par_range_model[which][0], cali->par_range_model[which][1]);
  }
  return logH;
}