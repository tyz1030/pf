#ifndef RBPF_H
#define RBPF_H

#include <functional>
#include <vector>
#include <array>
#include <Eigen/Dense>

#include "cf_filters.h" // for closed form filter objects


//! Rao-Blackwellized/Marginal Particle Filter with inner HMMs
/**
 * @class rbpf_hmm
 * @author t
 * @file rbpf.h
 * @brief Rao-Blackwellized/Marginal Particle Filter with inner HMMs
 * @tparam nparts the number of particles
 * @tparam dimnss dimension of "not sampled state"
 * @tparam dimss dimension of "sampled state"
 * @tparam dimy the dimension of the observations
 * @tparam resampT the resampler type (e.g. multinomial, etc.)
 */
template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
class rbpf_hmm{
public:

    /** "sampled state size vector" */
    using sssv = Eigen::Matrix<double,dimss,1>;
    /** "not sampled state size vector" */
    using nsssv = Eigen::Matrix<double,dimss,1>;
    /** "observation size vector" */
    using osv = Eigen::Matrix<double,dimy,1>;
//    /** "sampled state size matrix" */
//    using sssMat = Eigen::Matrix<double,dimss,dimss>;
    /** "not sampled state size matrix" */
    using nsssMat = Eigen::Matrix<double,dimnss,dimnss>;
    /** Dynamic size matrix*/
    using Mat = Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic>;
    /** array of model objects */
    using arrayMod = std::array<hmm<dimnss,dimy>,nparts>;
    /** array of samples */
    using arrayVec = std::array<sssv,nparts>;
    /** array of weights */
    using arrayDouble = std::array<double,nparts>;


    //! The constructor.
    /**
     * @brief constructor.
     * @param resamp_sched how often to resample (e.g. once every resamp_sched time periods)
     */
    rbpf_hmm(const unsigned int &resamp_sched);
    

    //! Filter.
    /**
     * @brief filters everything based on a new data point.
     * @param data the most recent time series observation.
     * @param fs a vector of functions computing E[h(x_1t, x_2t^i)| x_2t^i,y_1:t] to be averaged to yield E[h(x_1t, x_2t)|,y_1:t]. Will access the probability vector of x_1t
     */
    void filter(const osv &data,
                const std::vector<std::function<const Mat(const nsssv &x1tProbs, const sssv &x2t)> >& fs 
                    = std::vector<std::function<const Mat(const nsssv&, const sssv&)> >());//, const std::vector<std::function<const Mat(const Vec&)> >& fs);


    //! Get the latest conditional likelihood.
    /**
     * @brief Get the latest conditional likelihood.
     * @return the latest conditional likelihood.
     */
    double getLogCondLike() const;
    
    //!
    /**
     * @brief Get vector of expectations.
     * @return vector of expectations
     */
    std::vector<Mat> getExpectations() const;

    //! Evaluates the first time state density.
    /**
     * @brief evaluates mu.
     * @param x21 component two at time 1
     * @return a double evaluation
     */
    virtual double logMuEv(const sssv &x21) = 0;
    
    
    //! Sample from the first sampler.
    /**
     * @brief samples the second component of the state at time 1.
     * @param y1 most recent datum.
     * @return a Vec sample for x21.
     */
    virtual sssv q1Samp(const osv &y1) = 0;
    
    
    //! Provides the initial mean vector for each HMM filter object.
    /**
     * @brief provides the initial probability vector for each HMM filter object.
     * @param x21 the second state componenent at time 1.
     * @return a Vec representing the probability of each state element.
     */
    virtual nssv initHMMProbVec(const sssv &x21) = 0;
    
    
    //! Provides the transition matrix for each HMM filter object.
    /**
     * @brief provides the transition matrix for each HMM filter object.
     * @param x21 the second state component at time 1. 
     * @return a transition matrix where element (ij) is the probability of transitioning from state i to state j.
     */
    virtual nsssMat initHMMTransMat(const sssv &x21) = 0;

    //! Samples the time t second component. 
    /**
     * @brief Samples the time t second component.
     * @param x2tm1 the previous time's second state component.
     * @param yt the current observation.
     * @return a Vec sample of the second state component at the current time.
     */
    virtual sssv qSamp(const sssv &x2tm1, const osv &yt) = 0;
    
    
    //! Evaluates the proposal density of the second state component at time 1.
    /**
     * @brief Evaluates the proposal density of the second state component at time 1.
     * @param x21 the second state component at time 1 you sampled. 
     * @param y1 time 1 observation.
     * @return a double evaluation of the density.
     */
    virtual double logQ1Ev(const sssv &x21, const osv &y1) = 0;
    
    
    //! Evaluates the state transition density for the second state component.
    /**
     * @brief Evaluates the state transition density for the second state component.
     * @param x2t the current second state component.
     * @param x2tm1 the previous second state component.
     * @return a double evaluation.
     */
    virtual double logFEv(const sssv &x2t, const sssv &x2tm1) = 0;
    
    
    //! Evaluates the proposal density at time t > 1.
    /**
     * @brief Evaluates the proposal density at time t > 1. 
     * @param x2t the current second state component.
     * @param x2tm1 the previous second state component.
     * @param yt the current time series observation.
     * @return a double evaluation.
     */
    virtual double logQEv(const sssv &x2t, const sssv &x2tm1, const osv &yt ) = 0;
    
    
    //! How to update your inner HMM filter object at each time.
    /**
     * @brief How to update your inner HMM filter object at each time.
     * @param aModel a HMM filter object describing the conditional closed-form model.
     * @param yt the current time series observation.
     * @param x2t the current second state component.
     */
    virtual void updateHMM(hmm<dimnss,dimy> &aModel, const osv &yt, const sssv &x2t) = 0;

private:
    
    /** the current time period */
    unsigned int m_now;
    /** last conditional likelihood */
    double m_lastLogCondLike;
    /** resampling schedue */
    unsigned int m_rs;
    /** the array of inner closed-form models */ 
    arrayMod m_p_innerMods;
    /** the array of samples for the second state portion */
    arrayVec m_p_samps;
    /** the array of unnormalized log-weights */
    arrayDouble m_logUnNormWeights;
    /** the resampler object */
    resampT m_resampler;
    /** the vector of expectations */
    std::vector<Mat> m_expectations; 

};


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
rbpf_hmm<nparts,dimnss,dimss,dimy,resampT>::rbpf_hmm(const unsigned int &resamp_sched)
    : m_now(0)
    , m_lastLogCondLike(0.0)
    , m_rs(resamp_sched)
{
    std::fill(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
void rbpf_hmm<nparts,dimnss,dimss,dimy,resampT>::filter(const osv &data, const std::vector<std::function<const Mat(const nsssv &x1tProbs, const sssv &x2t)> >& fs)
{

    if( m_now == 0){ // first data point coming
    
        // initialize and update the closed-form mods        
        nsssv tmpProbs;
        nsssMat tmpTransMat;
        double m(-1.0/0.0);
        for(size_t ii = 0; ii < nparts; ++ii){
            
            m_p_samps[ii] = q1Samp(data); 
            tmpProbs = initHMMProbVec(m_p_samps[ii]);
            tmpTransMat = initHMMTransMat(m_p_samps[ii]);
            m_p_innerMods[ii] = hmm<dimnss,dimy>(tmpProbs, tmpTransMat);
            this->updateHMM(m_p_innerMods[ii], data, m_p_samps[ii]);
            m_logUnNormWeights[ii] = m_p_innerMods[ii].getLogCondLike() + logMuEv(m_p_samps[ii]) - logQ1Ev(m_p_samps[ii], data);

            // maximum to be used in likelihood calc
            if(m_logUnNormWeights[ii] > m)
                m = m_logUnNormWeights[ii];
        }

        // calc log p(y1)
        double sumexp(0.0);
        for(size_t p = 0; p < nparts; ++p)
            sumexp += std::exp(m_logUnNormWeights[p] - m);
        m_lastLogCondLike = m + std::log(sumexp) - std::log(static_cast<double>(nparts));

        // calculate expectations before you resample
        m_expectations.resize(fs.size());
        unsigned int fId(0);
        unsigned int dimOut;
        double m = *std::max_element(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
        for(auto & h : fs){

            dimOut = h(m_p_innerMods[0].getFilterVec(), m_p_samps[0]);
            Eigen::Mat<double,dimOut,dimOut> numer = Eigen::Mat<double,dimOut,dimOut>::Zero();
            Eigen::Mat<double,dimOut,dimOut> ones  = Eigen::Mat<double,dimOut,dimOut>::Ones();
            Eigen::Mat<double,dimOut,dimOut> tmp;
            double denom(0.0);
                        
            for(size_t prtcl = 0; prtcl < nparts; ++prtcl){ 
                tmp = h(m_p_innerMods[prtcl].getFilterVec(), m_p_samps[prtcl]);
                tmp = tmp.array().log().matrix() + (m_logUnNormWeights[prtcl] - m)*ones;
                numer = numer + tmp.array().exp().matrix();
                denom += std::exp( m_logUnNormWeights[prtcl] - m );
            }
            m_expectations[fId] = numer/denom;
            fId++;
        }
        
        // resample (unnormalized weights ok)
        if( (m_now+1) % m_rs == 0)
            m_resampler.resampLogWts(m_p_innerMods, m_p_samps, m_logUnNormWeights);

        // advance time step
        m_now ++;
    }
    else { //m_now > 0
        
        // update
        sssv newX2Samp;
        double sumexpdenom(0.0);
        double m1(-1.0/0.0); // for revised log weights
        double m2 = *std::max_element(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
        for(unsigned ii = 0; ii < nparts; ++ii){
            
            newX2Samp = qSamp(m_p_samps[ii], data);
            updateFSHMM(m_p_innerMods[ii], data, newX2Samp);
            sumexpdenom += std::exp(m_logUnNormWeights[ii] - m2);
            logUnNormWeightUpdate = m_p_innerMods[ii].getLogCondLike()
                                    + logFEv(newX2Samp, m_p_samps[ii]) 
                                    - logQEv(newX2Samp, m_p_samps[ii], data);
            
            // update a max
            if(m_logUnNormWeights[ii] > m1)
                m1 = m_logUnNormWeights[ii];
            
            m_p_samps[ii] = newX2Samp;
        }
        
        // calculate log p(y_t | y_{1:t-1})
        double sumexpnumer(0.0);
        for(size_t p = 0; p < nparts; ++p)
            sumexpnumer += std::exp(m_logUnNormWeights - m1);
        m_lastLogCondLike = m1 + std::log(sumexpnumer) - m2 - std::log(sumexpdenom);
        
        // calculate expectations before you resample
        unsigned int fId(0);
        unsigned int dimOut;
        double m = *std::max_element(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
        for(auto & h : fs){
            
            dimOut = h(m_p_innerMods[0].getFilterVec(), m_p_samps[0]);
            Eigen::Mat<double,dimOut,dimOut> numer = Eigen::Mat<double,dimOut,dimOut>::Zero();
            Eigen::Mat<double,dimOut,dimOut> ones  = Eigen::Mat<double,dimOut,dimOut>::Ones();
            Eigen::Mat<double,dimOut,dimOut> tmp;
            double denom(0.0);

            for(size_t prtcl = 0; prtcl < nparts; ++prtcl){ 
                tmp = h(m_p_innerMods[prtcl].getFilterVec(), m_p_samps[prtcl]);
                tmp = tmp.array().log().matrix() + (m_logUnNormWeights[prtcl] - m)*ones;
                numer = numer + tmp.array().exp().matrix();
                denom += std::exp( m_logUnNormWeights[prtcl] - m );
            }
            m_expectations[fId] = numer/denom;
            fId++;
        }

        // resample (unnormalized weights ok)
        if( (m_now+1) % m_rs == 0)
            m_resampler.resampLogWts(m_p_innerMods, m_p_samps, m_logUnNormWeights);
        
        // update time step
        m_now ++;
    }
    
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
double rbpf_hmm<nparts,dimnss,dimss,dimy,resampT>::getLogCondLike() const
{
    return m_lastLogCondLike;
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
auto rbpf_hmm<nparts,dimnss,dimss,dimy,resampT>::getExpectations() const -> std::vector<Mat>
{
    return m_expectations;
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
class rbpf_kalman{

public:

    /** "sampled state size vector" */
    using sssv = Eigen::Matrix<double,dimss,1>;
    /** "not sampled state size vector" */
    using nsssv = Eigen::Matrix<double,dimss,1>;
    /** "observation size vector" */
    using osv = Eigen::Matrix<double,dimy,1>;
//    /** "sampled state size matrix" */
//    using sssMat = Eigen::Matrix<double,dimss,dimss>;
    /** dynamic size matrices */
    using Mat = Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic>;
    /** "not sampled state size matrix" */
    using nsssMat = Eigen::Matrix<double,dimnss,dimnss>;
    /** array of model objects */
    using arrayMod = std::array<kalman<dimnss,dimobs>,nparts>;
    /** array of samples */
    using arrayVec = std::array<sssv,nparts>;
    /** array of weights */
    using arrayDouble = std::array<double,nparts>;

    //! The constructor.
    /**
     \param resamp_sched how often you want to resample (e.g once every resamp_sched time points)
     */
    rbpf_kalman(const unsigned int &resamp_sched);
    
    
    //! Filter! 
    /**
     * \brief The workhorse function
     * \param data the most recent observable portion of the time series.
     * \param fs a vector of functions computing E[h(x_1t, x_2t^i)| x_2t^i,y_1:t]. to be averaged to yield E[h(x_1t, x_2t)|,y_1:t]
     */
    void filter(const osv &data, const std::vector<std::function<const Mat(const nsssv &x1t, const sssv &x2t)> >& fs
                                     = std::vector<std::function<const Mat(const nsssv &x1t, const sssv &x2t)> >() );

    //! Get the latest log conditional likelihood.
    /**
     * \return the latest log conditional likelihood.
     */
    double getLogCondLike() const; 
    
    
    //! Get the latest filtered expectation E[h(x_1t, x_2t) | y_{1:t}]
    /**
     * @brief Get the expectations you're keeping track of.
     * @return a vector of Mats
     */
    std::vector<Mat> getExpectations() const;
    
    
    //! Evaluates the first time state density.
    /**
     * @brief evaluates log mu(x21).
     * @param x21 component two at time 1
     * @return a double evaluation
     */
    virtual double logMuEv(const sssv &x21) = 0;
    
    
    //! Sample from the first time's proposal distribution.
    /**
     * @brief samples the second component of the state at time 1.
     * @param y1 most recent datum.
     * @return a Vec sample for x21.
     */
    virtual ssv q1Samp(const osv &y1) = 0;
    
    
    //! Provides the initial mean vector for each Kalman filter object.
    /**
     * @brief provides the initial mean vector for each Kalman filter object.
     * @param x21 the second state componenent at time 1.
     * @return a nsssv representing the unconditional mean.
     */
    virtual nssv initKalmanMean(const sssv &x21) = 0;
    
    
    //! Provides the initial covariance matrix for each Kalman filter object.
    /**
     * @brief provides the initial covariance matrix for each Kalman filter object.
     * @param x21 the second state component at time 1.
     * @return a covariance matrix. 
     */
    virtual nsssMat initKalmanVar(const sssv &x21) = 0;
    
    
    //! Samples the time t second component. 
    /**
     * @brief Samples the time t second component.
     * @param x2tm1 the previous time's second state component.
     * @param yt the current observation.
     * @return a sssv sample of the second state component at the current time.
     */
    virtual sssv qSamp(const sssv &x2tm1, const osv &yt) = 0;
    
    
    //! Evaluates the proposal density of the second state component at time 1.
    /**
     * @brief Evaluates the proposal density of the second state component at time 1.
     * @param x21 the second state component at time 1 you sampled. 
     * @param y1 time 1 observation.
     * @return a double evaluation of the density.
     */
    virtual double logQ1Ev(const sssv &x21, const osv &y1) = 0;
    
    
    //! Evaluates the state transition density for the second state component.
    /**
     * @brief Evaluates the state transition density for the second state component.
     * @param x2t the current second state component.
     * @param x2tm1 the previous second state component.
     * @return a double evaluation.
     */
    virtual double logFEv(const sssv &x2t, const sssv &x2tm1) = 0;
    
    
    //! Evaluates the proposal density at time t > 1.
    /**
     * @brief Evaluates the proposal density at time t > 1. 
     * @param x2t the current second state component.
     * @param x2tm1 the previous second state component.
     * @param yt the current time series observation.
     * @return a double evaluation.
     */
    virtual double logQEv(const sssv &x2t, const sssv &x2tm1, const osv &yt) = 0;
    
    
    //! How to update your inner Kalman filter object at each time.
    /**
     * @brief How to update your inner Kalman filter object at each time.
     * @param aModel a Kalman filter object describing the conditional closed-form model.
     * @param yt the current time series observation.
     * @param x2t the current second state component.
     */
    virtual void updateKalman(kalman<dimnss, dimobs> &kMod, const osv &yt, const sssv &x2t) = 0;
    
private:

    /** the resamplign schedule */
    unsigned int m_rs;
    /** the array of inner Kalman filter objects */
    arrayMod m_p_innerMods;
    /** the array of particle samples */
    arrayVec m_p_samps;
    /** the array of the (log of) unnormalized weights */
    arrayDouble m_logUnNormWeights;
    /** the current time period */
    unsigned int m_now;
    /** log p(y_t|y_{1:t-1}) or log p(y1) */
    double m_lastLogCondLike; 
    /** resampler object */
    resampT m_resampler;
    /** expectations */
    std::vector<Mat> m_expectations;
};


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
rbpf_kalman<nparts,dimnss,dimss,dimy,reampT>::rbpf_kalman(const unsigned int &resamp_sched)
    : m_now(0)
    , m_lastLogCondLike(0.0)
    , m_rs(resamp_sched)
{
    std::fill(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
void rbpf_kalman<nparts,dimnss,dimss,dimy,reampT>::filter(const osv &data, const std::vector<std::function<const Mat(const nsssv &x1t, const sssv &x2t)> >& fs)
{
    
    if( m_now == 0){ // first data point coming
    
        // initialize and update the closed-form mods      
        nsssv tmpMean;
        nsssMat tmpVar;
//        double logWeightAdj;
//        double tmpForFirstLike(0.0);
        double m1(-1.0/0.0);
        for(size_t ii = 0; ii < nparts; ++ii){
            m_p_samps[ii] = q1Samp(data); 
            tmpMean = initKalmanMean(m_p_samps[ii]);
            tmpVar  = initKalmanVar(m_p_samps[ii]);
            m_p_innerMods[ii] = kalman<dimnss,dimobs,1>(tmpMean, tmpVar);   // TODO: allow for input or check to make sure this doesn't break anything else
            this->updateKalman(m_p_innerMods[ii], data, m_p_samps[ii]);

            m_logUnNormWeights[ii] = m_p_innerMods[ii].getLogCondLike() + logMuEv(m_p_samps[ii]) - logQ1Ev(m_p_samps[ii], data);

            // update a max
            if(m_logUnNormWeights[ii] > m1)
                m1 = m_logUnNormWeights[ii];
        }

        // calculate log p(y1)
        double sumexp(0.0);
        for(size_t p = 0; p < nparts; ++p)
            sumexp += std::exp(m_logUnNormWeights[p] - m1);
        m_logLastCondLike = m1 + std::log(sumexp) - std::log(static_cast<double>(nparts));  

        // calculate expectations before you resample
        m_expectations.resize(fs.size());
        unsigned int fId(0);
        unsigned int dimOut;
        double m = *std::max_element(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
        for(auto & h : fs){

            dimOut = h(m_p_innerMods[0].getFilterVec(), m_p_samps[0]);
            Eigen::Mat<double,dimOut,dimOut> numer = Eigen::Mat<double,dimOut,dimOut>::Zero();
            Eigen::Mat<double,dimOut,dimOut> ones  = Eigen::Mat<double,dimOut,dimOut>::Ones();
            Eigen::Mat<double,dimOut,dimOut> tmp;
            double denom(0.0);

            for(size_t prtcl = 0; prtcl < nparts; ++prtcl){ 
                tmp = h(m_p_innerMods[prtcl].getFilterVec(), m_p_samps[prtcl]);
                tmp = tmp.array().log().matrix() + (m_logUnNormWeights[prtcl] - m)*ones;
                numer = numer + tmp.array().exp().matrix();
                denom += std::exp( m_logUnNormWeights[prtcl] - m );
            }
            m_expectations[fId] = numer/denom;
            fId++;
        }
        
        // resample (unnormalized weights ok)
        if( (m_now+1)%m_rs == 0)
            m_resampler.resampLogWts(m_p_innerMods, m_p_samps, m_logUnNormWeights)
            
        // advance time step
        m_now ++;
    }
    else { //m_now > 0
        
        // update
        sssv newX2Samp;
        double m1(-1.0/0.0); // for updated weights
        double m2 = *std::max_element(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
        double sumexpdenom(0.0);
        for(size_t ii = 0; ii < nparts; ++ii){
            newX2Samp = qSamp(m_p_samps[ii], data);
            this->updateKalman(m_p_innerMods[ii], data, newX2Samp);

            // before you update the weights
            sumexpdenom += std::exp(m_logUnNormWeights[ii] - m2);
            
            // update the weights
            m_logUnNormWeights[ii] += m_p_innerMods[ii].getLogCondLike() * logFEv(newX2Samp, m_p_samps[ii]) - logQEv(newX2Samp, m_p_samps[ii], data);
            
            // update a max
            if(m_logUnNormWeights[ii] > m1)
                m1 = m_logUnNormWeights[ii];
                
            m_p_samps[ii] = newX2Samp;
        }
        
        // calc log p(y_t | y_{1:t-1})
        double sumexpnumer(0.0);
        for(size_t p = 0; p < nparts; ++p)
            sumexpnumer += std::exp(m_logUnNormWeights[p] - m1);
        m_lastCondLike = m1 + std::log(sumexpnumer) - m2 - std::log(sumexpdenom);
        
        // calculate expectations before you resample
        unsigned int fId(0);
        unsigned int dimOut;
        double m = *std::max_element(m_logUnNormWeights.begin(), m_logUnNormWeights.end());
        for(auto & h : fs){

            dimOut = h(m_p_innerMods[0].getFilterVec(), m_p_samps[0]);
            Eigen::Mat<double,dimOut,dimOut> numer = Eigen::Mat<double,dimOut,dimOut>::Zero();
            Eigen::Mat<double,dimOut,dimOut> ones  = Eigen::Mat<double,dimOut,dimOut>::Ones();
            Eigen::Mat<double,dimOut,dimOut> tmp;
            double denom(0.0);

            for(size_t prtcl = 0; prtcl < nparts; ++prtcl){ 
                tmp = h(m_p_innerMods[prtcl].getFilterVec(), m_p_samps[prtcl]);
                tmp = tmp.array().log().matrix() + (m_logUnNormWeights[prtcl] - m)*ones;
                numer = numer + tmp.array().exp().matrix();
                denom += std::exp( m_logUnNormWeights[prtcl] - m );
            }
            m_expectations[fId] = numer/denom;
            fId++;
        }

        // resample (unnormalized weights ok)
        if( (m_now+1)%m_rs == 0)
            m_resampler.resampLogWts(m_p_innerMods, m_p_samps, m_logUnNormWeights)
        
        // update time step
        m_now ++;
    }
 
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
double rbpf_kalman<nparts,dimnss,dimss,dimy,reampT>::getLogCondLike() const
{
    return m_lastLogCondLike;
}


template<size_t nparts, size_t dimnss, size_t dimss, size_t dimy, typename resampT>
auto rbpf_kalman<nparts,dimnss,dimss,dimy,reampT>::getExpectations() const -> std::vector<Mat>
{
    return m_expectations;
}


#endif //RBPF_H
