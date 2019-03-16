#include <algorithm>


void pbil::educate() {		   
  std::uniform_real_distribution<double> dist(0, 1);
  for (auto& row : samples) {
    unsigned b = 0;    
    for (auto& c : row) {
      c = int(dist(rng) < probabilities[b++]);
    }    
  }
}


// warm up pbil (localize search around initial best guess)
void pbil::initialize_probabilities() {
  double eps = 1e-2;
  std::uniform_real_distribution<double> dist(0, eps);
  std::vector<int> tmp(initial_guess);

  for (auto& g : tmp) g += dist(rng);


  for (unsigned b = 0; b < probabilities.size(); ++b) {

    double lr = learn_rate + neg_learn_rate;

    probabilities[b] = probabilities[b] * (1 - lr) + tmp[b] * lr;
  }
}


void pbil::update_probabilities(const std::vector<int>& min_gene,
				const std::vector<int>& max_gene) {
  
  for (unsigned b = 0; b < probabilities.size(); ++b) {

    double lr = (min_gene[b] == max_gene[b] ?
      learn_rate : learn_rate + neg_learn_rate);

    probabilities[b] = probabilities[b] * (1 - lr) + min_gene[b] * lr;
  }
}


void pbil::mutate() {

  std::uniform_real_distribution<double> dist(0, 1);

  for (auto& p : probabilities) {
    
    if (dist(rng) < mutate_prob) {
      
      p = p * (1.0 - mutate_shift)
        + (dist(rng) < 0.5 ? 1.0 : 0) * mutate_shift;
      
    }    
  }
}


void pbil::init() {
  
  std::for_each(samples.begin(), samples.end(),
		[](std::vector<int>& e) { std::fill(e.begin(), e.end(), 0); });
  
  std::fill(probabilities.begin(), probabilities.end(), 0.5);

}


template<class T, typename... Args>
void pbil::optimize(T&& residual, Args&&... args) {
  using namespace std::placeholders;
  
  init();
  
  if (initial_guess.size() > 0)
    for (size_t i = 0; i < 6; ++i) initialize_probabilities();

  std::vector<int> best;
  
  auto rf =
    std::bind(std::forward<T>(residual),
      _1,
      std::ref(std::forward<Args>(args))...);
  
  while (best_err >= etol) {
    
    educate();
    
    
    int i = 0;
    std::vector<double> errors(samples.size());
    
    for (auto& e : errors) { e = rf(samples[i++]); }
    
    
    double min_err = 1e10;
    double max_err = -1e10;
    std::vector<int> min_sample, max_sample;
    
    i = 0;
    for (const auto& e : errors) {
      if (min_err > e) {
        min_err = e;
        min_sample = samples[i];
      }
      if (max_err < e) {
        max_err = e;
        max_sample = samples[i];
      }
      ++i;
    }
    
    if (best_err > min_err) {
      best_err = min_err;
      best = min_sample;
      
      for (auto& b : best) {
        std::cout << b;
      }
      std::cout << std::endl;
    }
    
    update_probabilities(min_sample, max_sample);
    mutate();
    
  }
}
