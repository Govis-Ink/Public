class TheFuncEval:
    import numpy as np
    import scipy.special as sc
    import scipy.stats as st
    from scipy.differentiate import derivative
    from scipy.integrate import cumulative_simpson

    @staticmethod
    def sim(f, a, b, x_points:int=200000):
        np = TheFuncEval.np
        x_points = max(100, np.abs(x_points))

        a = np.asarray(a)
        b = np.asarray(b)
        scalar = a.ndim == 0 and b.ndim == 0
        a = np.atleast_1d(a)
        b = np.atleast_1d(b)
        
        x_min = min(np.min(a), np.min(b))
        x_max = max(np.max(a), np.max(b))
        x_fine = np.linspace(x_min, x_max, x_points)
        fx_fine = f(x_fine)
        sim_fx = TheFuncEval.cumulative_simpson(fx_fine, x=x_fine, initial=0)

        sim_fa = np.interp(a, x_fine, sim_fx)
        sim_fb = np.interp(b, x_fine, sim_fx)
        res = sim_fb - sim_fa

        if scalar:
            return res[0]
        return res
    
    @staticmethod
    def cum(f, a, b, h=1, x_points:int=100000):
        np = TheFuncEval.np
        x_points = max(100, np.abs(x_points))

        a = np.asarray(a)
        b = np.asarray(b)
        scalar = a.ndim == 0 and b.ndim == 0
        a = np.atleast_1d(a)
        b = np.atleast_1d(b)
        
        x_min = min(np.min(a), np.min(b))
        x_max = max(np.max(a), np.max(b))
        margin = 3 * (x_max - x_min) / (x_points-1)
        x_fine = np.linspace(x_min-margin, x_max+margin, x_points)
        fx_fine = f(x_fine)

        fx_der1 = np.zeros(x_points)
        fx_der3 = np.zeros(x_points)

        dx = x_fine[1] - x_fine[0]
        fx_der1[3:-3] = (-fx_fine[6:] + 9*fx_fine[5:-1] - 45*fx_fine[4:-2] + 45*fx_fine[2:-4] - 9*fx_fine[1:-5] + fx_fine[:-6]) / (60*dx)
        fx_der3[3:-3] = (-fx_fine[6:] + 8*fx_fine[5:-1] - 13*fx_fine[4:-2] + 13*fx_fine[2:-4] - 8*fx_fine[1:-5] + fx_fine[:-6]) / (2*dx**3)

        sim_fx = TheFuncEval.cumulative_simpson(fx_fine, x=x_fine, initial=0)
        cum_fx = sim_fx / h + fx_fine / 2 + fx_der1 * h / 12 - fx_der3 * h ** 3 / 720

        cum_fa = np.interp(a, x_fine, cum_fx)
        cum_fb = np.interp(b, x_fine, cum_fx)
        res = cum_fb - cum_fa

        if scalar:
            return res[0]
        return res
    
    @staticmethod
    def sder(f, x, x_points:int=3000000):
        np = TheFuncEval.np

        if isinstance(x, (int, float)):
            return TheFuncEval.derivative(f, x).df
        
        x_min, x_max = np.min(x), np.max(x)
        margin = 4 * (x_max - x_min) / (x_points-1)
        x_fine = np.linspace(x_min-margin, x_max+margin, x_points)
        fx_fine = f(x_fine)

        fx_der = np.zeros(x_points)
        dx = x_fine[1] - x_fine[0]
        fx_der[4:-4] = (- 27*fx_fine[8:] + 448*fx_fine[7:-1] - 2880*fx_fine[6:-2] + 12096*fx_fine[5:-3]
                        - 12096*fx_fine[3:-5] + 2880*fx_fine[2:-6] - 448*fx_fine[1:-7] + 27*fx_fine[:-8]) / (15120*dx)
        
        return np.interp(x, x_fine, fx_der)

    @staticmethod
    def der_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda x: 0
        elif res == 'func':
            return lambda x: TheFuncEval.derivative(f, x).df
        else:
            return None
        
    @staticmethod
    def sder_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda x: 0
        elif res == 'func':
            return lambda x: TheFuncEval.sder(f, x)
        else:
            return None

    @staticmethod
    def diff_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda x, step=1: 0
        elif res == 'func':
            return lambda x, step=1: f(x+step) - f(x)
        else:
            return None

    @staticmethod
    def difb_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda x, step=1: 0
        elif res == 'func':
            return lambda x, step=1: f(x) - f(x-step)
        else:
            return None

    @staticmethod
    def sim_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda a, b: f * (b - a)
        elif res == 'func':
            return lambda a, b: TheFuncEval.sim(f, a, b)
        else:
            return None

    @staticmethod
    def cum_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda a, b, step=1: f * (b - a)
        elif res == 'func':
            return lambda a, b, step=1: TheFuncEval.cum(f, a, b, step)
        else:
            return None
        
    @staticmethod
    def ddif_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda x, step=1: 0
        elif res == 'func':
            return lambda x, step=1: (f(x+step) - f(x-step)) / step / 2
        else:
            return None
        
    @staticmethod
    def dcum_func(func_str: str):
        res, f = TheFuncEval.func_eval(func_str, ['t'])
        if res == 'num':
            return lambda a, b, step=1: f * (b - a)
        elif res == 'func':
            return lambda a, b, step=1: TheFuncEval.cum(f, a, b, step) *  step
        else:
            return None
    
    __func_dict = {
        '__builtins__':None,
        'int':int, 'float':float,
        'e':np.e, 'pi':np.pi, 'nan':np.nan, 'inf':np.inf,
        'sqrt':np.sqrt, 'cbrt':np.cbrt, 'sinc':np.sinc,
        'sin':np.sin, 'cos':np.cos, 'tan':np.tan,
        'arcsin':np.arcsin, 'arccos':np.arccos, 'arctan':np.arctan,
        'asin':np.asin, 'acos':np.acos, 'atan':np.atan,
        'sinh':np.sinh, 'cosh':np.cosh, 'tanh':np.tanh,
        'arcsinh':np.arcsinh, 'arccosh':np.arccosh, 'arctanh':np.arctanh,
        'asinh':np.asinh, 'acosh':np.acosh, 'atanh':np.atanh,
        'deg':np.degrees, 'rad':np.radians,
        'round':np.round, 'floor':np.floor, 'ceil':np.ceil, 'trunc':np.trunc,
        'exp':np.exp, 'expm1':np.expm1, 'exp2':np.exp2,
        'log':np.log, 'log1p':np.log1p, 'log2':np.log2, 'log10':np.log10,
        'min':np.min, 'max':np.max,
        'lcm':np.lcm, 'gcd':np.gcd,
        'add':np.add, 'rec':np.reciprocal,
        'positive':np.positive, 'negative':np.negative, 'sign':np.sign, 'signbit':np.signbit, 'fabs':np.fabs, 'abs':np.abs,
        'mul':np.multiply, 'div':np.divide, 'pow':np.power, 'fmod':np.fmod, 'mod':np.mod,
        'der':der_func, 'sim':sim_func, 'sder':sder_func,
        'diff':diff_func, 'difb':difb_func, 'cum':cum_func, 'ddif':ddif_func, 'dcum':dcum_func,
        'jv':sc.jv, 'jve':sc.jve, 'yn':sc.yn, 'yv':sc.yv, 'yve':sc.yve,
        'iv':sc.iv, 'ive':sc.ive, 'kn':sc.kn, 'kv':sc.kv, 'kve':sc.kve,
        'j0':sc.j0, 'j1':sc.j1, 'y0':sc.y0, 'y1':sc.y1,
        'i0':sc.i0, 'i0e':sc.i0e, 'i1':sc.i1, 'i1e':sc.i1e,
        'k0':sc.k0, 'k0e':sc.k0e, 'k1':sc.k1, 'k1e':sc.k1e,
        'struve':sc.struve, 'modstruve':sc.modstruve, 'itstruve0':sc.itstruve0,
        'it2struve0':sc.it2struve0, 'itmodstruve0':sc.itmodstruve0,
        'bdtr':sc.bdtr, 'bdtrc':sc.bdtrc, 'bdtri':sc.bdtri, 'bdtrik':sc.bdtrik, 'bdtrin':sc.bdtrin,
        'btdtria':sc.btdtria, 'btdtrib':sc.btdtrib,
        'fdtr':sc.fdtr, 'fdtrc':sc.fdtrc, 'fdtri':sc.fdtri, 'fdtridfd':sc.fdtridfd,
        'gdtr':sc.gdtr, 'gdtrc':sc.gdtrc, 'gdtria':sc.gdtria, 'gdtrib':sc.gdtrib, 'gdtrix':sc.gdtrix,
        'nbdtr':sc.nbdtr, 'nbdtrc':sc.nbdtrc, 'nbdtri':sc.nbdtri, 'nbdtrik':sc.nbdtrik, 'nbdtrin':sc.nbdtrin,
        'ncfdtr':sc.ncfdtr, 'ncfdtridid':sc.ncfdtridfd, 'ncfdtridfn':sc.ncfdtridfn, 'ncfdtri':sc.ncfdtri, 'ncfdtrinc':sc.ncfdtrinc,
        'nctdtr':sc.nctdtr, 'nctdtridf':sc.nctdtridf, 'nctdtrit':sc.nctdtrit, 'nctdtrinc':sc.nctdtrinc,
        'nrdtrimn':sc.nrdtrimn, 'nrdtrisd':sc.nrdtrisd, 'ndtr':sc.ndtr, 'logndtr':sc.log_ndtr, 'ndtri':sc.ndtri, 'ndtrie':sc.ndtri_exp,
        'pdtr':sc.pdtr, 'pdtrc':sc.pdtrc, 'pdtri':sc.pdtri, 'pdtrik':sc.pdtrik,
        'stdtr':sc.stdtr, 'stdtridf':sc.stdtridf, 'stdtrit':sc.stdtrit,
        'chdtr':sc.chdtr, 'chdtrc':sc.chdtrc, 'chdtri':sc.chdtri, 'chdtriv':sc.chdtriv,
        'chndtr':sc.chndtr, 'chndtridf':sc.chndtridf, 'chndtrinc':sc.chndtrinc, 'chndtrix':sc.chndtrix,
        'smirnov':sc.smirnov, 'smirnovi':sc.smirnovi, 'kolmogorov':sc.kolmogorov, 'kolmogi':sc.kolmogi,
        'boxcox':sc.boxcox, 'boxcox1p':sc.boxcox1p, 'invboxcox':sc.inv_boxcox, 'invboxcox1p':sc.inv_boxcox1p,
        'logit':sc.logit, 'expit':sc.expit, 'logexpit':sc.log_expit,
        'tklambda':sc.tklmbda, 'owens':sc.owens_t,
        'entr':sc.entr, 'relentr':sc.rel_entr, 'kldiv':sc.kl_div, 'huber':sc.huber, 'phuber':sc.pseudo_huber,
        'gamma':sc.gamma, 'gammaln':sc.gammaln, 'loggamma':sc.loggamma, 'gammasgn':sc.gammasgn,
        'gammainc':sc.gammainc, 'gammaincinv':sc.gammaincinv,
        'gammaincc':sc.gammaincc, 'gammainccinv':sc.gammainccinv,
        'beta':sc.beta, 'betaln':sc.betaln, 'betainc':sc.betainc, 
        'betaincc':sc.betaincc, 'betaincinv':sc.betaincinv, 'betainccinv':sc.betainccinv,
        'psi':sc.psi, 'rgamma':sc.rgamma, 'digamma':sc.digamma, 'poch':sc.poch,
        'erf':sc.erf, 'erfc':sc.erfc, 'erfcx':sc.erfcx, 'erfi':sc.erfi, 'erfinv':sc.erfinv,
        'erfcinv':sc.erfcinv, 'dawns':sc.dawsn, 'voigt':sc.voigt_profile, 'lpmv':sc.lpmv,
        'legendre':sc.eval_legendre, 'chebyt':sc.eval_chebyt, 'chebyu':sc.eval_chebyu, 'chebyc':sc.eval_chebyc,
        'chebys':sc.eval_chebys, 'jacobi':sc.eval_jacobi, 'laguerre':sc.eval_laguerre, 'genlaguerre':sc.eval_genlaguerre,
        'hermite':sc.eval_hermite, 'hermitenorm':sc.eval_hermitenorm, 'gegenbauer':sc.eval_gegenbauer, 'shlegendre':sc.eval_sh_legendre, 
        'shchebyt':sc.eval_sh_chebyt, 'shchebyu':sc.eval_sh_chebyu, 'shjacobi':sc.eval_sh_jacobi,
        'hyp2f1':sc.hyp2f1, 'hyp1f1':sc.hyp1f1, 'hyperu':sc.hyperu, 'hyp0f1':sc.hyp0f1,
        'mathieua':sc.mathieu_a, 'mathieub':sc.mathieu_b, 'mathieucem':sc.mathieu_cem, 'mathieusem':sc.mathieu_sem,
        'mathieumodcem1':sc.mathieu_modcem1, 'mathieumodcem2':sc.mathieu_modcem2,
        'mathieumodsem1':sc.mathieu_modcem1, 'mathieumodsem2':sc.mathieu_modsem2,
        'proang1':sc.pro_ang1, 'prorad1':sc.pro_rad1, 'prorad2':sc.pro_rad2,
        'oblang1':sc.obl_ang1, 'oblrad1':sc.obl_rad1, 'oblrad2':sc.obl_rad2,
        'procv':sc.pro_cv, 'oblcv':sc.obl_cv,
        'proang1cv':sc.pro_ang1_cv, 'prorad1cv':sc.pro_rad1_cv, 'prorad2cv':sc.pro_rad2_cv,
        'bolang1cv':sc.obl_ang1_cv, 'oblrad1cv':sc.obl_rad1_cv, 'oblrad2cv':sc.obl_rad2_cv,
        'kelvin':sc.kelvin, 'ber':sc.ber, 'bei':sc.bei, 'berp':sc.berp,
        'beip':sc.beip, 'ker':sc.ker, 'kei':sc.kei, 'kerp':sc.kerp, 'keip':sc.keip,
        'agm':sc.agm, 'binom':sc.binom, 'expn':sc.expn, 'spence':sc.spence,'zeta':sc.zetac,
        'sindg':sc.sindg, 'cosdg':sc.cosdg, 'tandg':sc.tandg, 'cotdg':sc.cotdg,
        'powm1':sc.powm1, 'xlogy':sc.xlogy, 'xlog1py':sc.xlog1py, 'exprel':sc.exprel,
        'pdf':lambda stats: stats.pdf, 'logpdf':lambda stats: stats.logpdf,
        'cdf':lambda stats: stats.cdf, 'logcdf':lambda stats: stats.logcdf,
        'sf':lambda stats: stats.sf, 'logsf':lambda stats: stats.logsf,
        'ppf':lambda stats: stats.ppf, 'isf':lambda stats: stats.isf,
        'norm':st.norm, 'cauchy':st.cauchy, 'laplace':st.laplace,
        'logistic':st.logistic, 'skewnorm':st.skewnorm, 'genlogistic':st.genlogistic,  
        'levy':st.levy, 't':st.t, 'cosine':st.cosine, 'arcsine':st.arcsine,
        'dgamma':st.dgamma, 'powernorm':st.powernorm, 'kappa4':st.kappa4, 'kappa3':st.kappa3,
        'alpha':st.alpha, 'anglit':st.anglit, 'argus':st.argus, 'betas':st.beta,
        'betaprime':st.betaprime, 'bradford':st.bradford, 'byrr':st.burr,
        'chi':st.chi, 'chi2':st.chi2, 'crystallball':st.crystalball, 'dweibull':st.dweibull,
        'erlang':st.erlang, 'expon':st.expon, 'exponnorm':st.exponnorm, 'exponweib':st.exponweib,
        'exponpow':st.exponpow, 'f':st.f, 'fatiguelife':st.fatiguelife, 'fisk':st.fisk,
        'foldnorm':st.foldnorm, 'genlogistic':st.genlogistic, 'gennorm':st.gennorm,
        'genpareto':st.genpareto, 'genexpon':st.genexpon, 'genextreme':st.genextreme,
        'gausshyper':st.gausshyper, 'gengamma':st.gengamma, 'genhalflogistic':st.genhalflogistic,
        'genhyperbolic':st.genhyperbolic, 'geninvgauss':st.geninvgauss, 'gibrat':st.gibrat,
        'gompertz':st.gompertz, 'gumbelr':st.gumbel_r, 'gumbell':st.gumbel_l,
        'halfcauchy':st.halfcauchy, 'halflogistic':st.halflogistic, 'halfnorm':st.halfnorm,
        'halfgennorm':st.halfgennorm, 'hypsecant':st.hypsecant, 'invgamma':st.invgamma,
        'invguass':st.invgauss, 'invweibull':st.invweibull, 'irwinhall':st.irwinhall,
        'jfskewt':st.jf_skew_t, 'johnsonsb':st.johnsonsb, 'johnsonsu':st.johnsonsu,
        'ksone':st.ksone, 'kstwo':st.kstwo, 'kstwobign':st.kstwobign,
        'landau':st.landau, 'levyl':st.levy_l, 'levys':st.levy_stable, 
        'lomax':st.lomax, 'maxwell':st.maxwell, 'mielke':st.mielke,
        'moyal':st.moyal, 'nakagami':st.nakagami, 'ncx2':st.ncx2,
        'ncf':st.ncf, 'nct':st.nct, 'norminvguass':st.norminvgauss,
        'pareto':st.pareto, 'pearson3':st.pearson3, 'powerlaw':st.powerlaw,
        'powerlognorm':st.powerlognorm, 'powernorm':st.powernorm, 'rdist':st.rdist,
        'rayleigh':st.rayleigh, 'rice':st.rice, 'recipinvguass':st.recipinvgauss,
        'relbreitwigner':st.rel_breitwigner, 'semicicular':st.semicircular,
        'skewcauthy':st.skewcauchy, 'trapezoid':st.trapezoid, 'triang':st.triang,
        'truncexpon':st.truncexpon, 'truncnorm':st.truncnorm, 'truncpareto':st.truncpareto,
        'truncweibullmin':st.truncweibull_min, 'tukeylambda':st.tukeylambda,
        'uniform':st.uniform, 'vonmises':st.vonmises, 'vonmisesline':st.vonmises_line,
        'wald':st.wald, 'weibullmin':st.weibull_min, 'weibullmax':st.weibull_max, 'wrapcauchy':st.wrapcauchy
    }

    @staticmethod
    def func_eval(func_str: str, param_names):

        func_str = func_str.strip()
        if not func_str:
            return 'fail', None
        
        fail, func_str = TheFuncEval._quote_func_arguments(func_str)
        if fail:
            return 'fail', None
        
        try:
            f = eval(func_str, TheFuncEval.__func_dict)
            if type(f) == int or type(f) == float:
                return 'num', f
        except:
            pass

        params = ', '.join(param_names)
        lambda_str = f"lambda {params} : {func_str}"
        try:
            f = eval(lambda_str, TheFuncEval.__func_dict)
            return 'func', f
        except:
            return 'fail', None
        
    @staticmethod
    def _quote_func_arguments(func_str: str):

        func_func_set = {'der', 'sim', 'sder', 'diff', 'difb', 'cum', 'ddif', 'dcum'}
        valid_chars = set('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_')

        outside = True
        current = []
        quoted = []
        stack = []

        for ch in func_str:
            if ch in valid_chars:
                current.append(ch)
                quoted.append(ch)
            else:
                if ch == '(':
                    current_str = ''.join(current)
                    if current_str in func_func_set and outside:
                        quoted.append('("')
                        stack.append(1)
                        outside = False
                    else:
                        quoted.append('(')
                        stack.append(0)
                elif ch == ')':
                    if len(stack) == 0:
                        return True, ''
                    last_brackets = stack.pop()
                    if last_brackets == 1:
                        quoted.append('")')
                        outside = True
                    else:
                        quoted.append(')')
                else:
                    quoted.append(ch)
                current.clear()
        
        if len(stack) != 0:
            return True, ''
        else:
            return False, ''.join(quoted)
        
    def der_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda x: 0
        elif res == 'func':
            return lambda x: TheFuncEval.derivative(f, x).df
        else:
            return None
        
    def sder_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda x: 0
        elif res == 'func':
            return lambda x: TheFuncEval.sder(f, x)
        else:
            return None

    def diff_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda x, step=1: 0
        elif res == 'func':
            return lambda x, step=1: f(x+step) - f(x)
        else:
            return None

    def difb_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda x, step=1: 0
        elif res == 'func':
            return lambda x, step=1: f(x) - f(x-step)
        else:
            return None

    def sim_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda a, b: f * (b - a)
        elif res == 'func':
            return lambda a, b: TheFuncEval.sim(f, a, b)
        else:
            return None

    def cum_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda a, b, step=1: f * (b - a)
        elif res == 'func':
            return lambda a, b, step=1: TheFuncEval.cum(f, a, b, step)
        else:
            return None
        
    def ddif_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda x, step=1: 0
        elif res == 'func':
            return lambda x, step=1: (f(x+step) - f(x-step)) / step / 2
        else:
            return None
        
    def dcum_func_s(self, func_str: str):
        res, f = self.func_eval_s(func_str, ['t'])
        if res == 'num':
            return lambda a, b, step=1: f * (b - a)
        elif res == 'func':
            return lambda a, b, step=1: TheFuncEval.cum(f, a, b, step) *  step
        else:
            return None
        
    def __init__(self, max_depth:int=30):
        self.__func_dict = TheFuncEval.__func_dict.copy()
        self.__func_dict.update(zip(
            ['der', 'sim', 'sder', 'diff', 'difb', 'cum', 'ddif', 'dcum'],
            [self.der_func_s, self.sim_func_s, self.sder_func_s, self.diff_func_s, self.difb_func_s, self.cum_func_s, self.ddif_func_s, self.dcum_func_s]
        ))
        self._recursion_depth = 0
        self._max_depth = abs(max_depth)

    def func_eval_s(self, func_str: str, param_names):

        func_str = func_str.strip()
        if not func_str:
            return 'fail', None
        
        fail, func_str = TheFuncEval._quote_func_arguments(func_str)
        if fail:
            return 'fail', None
    
        try:
            f = eval(func_str, self.__func_dict)
            if type(f) == int or type(f) == float:
                return 'num', f
        except:
            pass

        params = ', '.join(param_names)
        lambda_str = f"lambda {params} : {func_str}"
        try:
            f = eval(lambda_str, self.__func_dict)
            return 'func', f
        except:
            return 'fail', None
        
    def func_add_s(self, func_str: str, param_names, func_name: str):

        res, f = self.func_eval_s(func_str, param_names)

        if res != 'fail' and func_name not in TheFuncEval.__func_dict:

            def depth_limited_f(*args, **kwargs):
                self._recursion_depth += 1
                try:
                    if self._recursion_depth > self._max_depth:
                        raise RecursionError(f"Recursion depth limit exceeded ({self._max_depth}) in function '{func_name}'")
                    return f(*args, **kwargs)
                finally:
                    self._recursion_depth -= 1

            self.__func_dict[func_name] = depth_limited_f
            
        return res, f
    
    def func_del_s(self, func_name: str):
        
        if func_name not in TheFuncEval.__func_dict and func_name in self.__func_dict:
            del self.__func_dict[func_name]

    def func_restore_s(self):

        self.__func_dict = TheFuncEval.__func_dict.copy()

    def func_name_free_s(self, func_name: str):

        return func_name not in self.__func_dict
    
    def func_disabled_s(self, func_name:str):

        if func_name in TheFuncEval.__func_dict and func_name in self.__func_dict:
            del self.__func_dict[func_name]

    def funcs_disabled_s(self, func_names:list[str]):
        
        for func_name in func_names:
            self.func_disabled_s(func_name)

    def func_enabled_s(self, func_name:str):

        if func_name in TheFuncEval.__func_dict and func_name not in self.__func_dict:
            self.__func_dict[func_name] = TheFuncEval.__func_dict[func_name]

    def funcs_enabled_s(self, func_names:list[str]):

        for func_name in func_names:
            self.func_enabled_s(func_name)

if __name__ == '__main__':
    
    np = TheFuncEval.np

    x = np.linspace(-10, 10, 100)

    res, f = TheFuncEval.func_eval('sim(t)(0,x)', ['x'])
    print(res, f(x))

    tfe = TheFuncEval()
    res, f = tfe.func_add_s('1/x', ['x'], 'g')
    print(res, f(x))

    res, f = tfe.func_eval_s('sder(g(t))(x)', ['x'])
    print(res, f(x))

    tfe.func_del_s('g')
    res, f = tfe.func_eval_s('cum(g(t))(0,x)', ['x'])