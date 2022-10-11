#include <interrupt/impl/shared_irq_impl.hpp>
#include <interrupt/builder/sub_irq_builder.hpp>
#include <interrupt/builder/shared_sub_irq_builder.hpp>

#include <interrupt/fwd.hpp>
#include <interrupt/config/fwd.hpp>

#include <type_traits>
#include <boost/hana.hpp>


#ifndef CIB_INTERRUPT_BUILDER_SHARED_IRQ_BUILDER_HPP
#define CIB_INTERRUPT_BUILDER_SHARED_IRQ_BUILDER_HPP


namespace interrupt {
    /**
     * Declare a shared interrupt with one or more SubIrqs.
     *
     * A shared interrupt declares one hardware irq that may be caused by one or more different
     * sub-interrupts. When a shared_irq is triggered, it will determine which sub_irq needs to be
     * triggered.
     *
     * This object is designed only to live in a constexpr context. The template specialization
     * should be declared by the user while the interrupt::Manager creates and manages
     * instances of shared_irq.
     */
    template<typename ConfigT>
    struct shared_irq_builder {
        constexpr static auto children = ConfigT::children;


        constexpr static auto irqs_type =
            hana::transform(ConfigT::children, [](auto child){
                if constexpr (hana::size(child.children) > hana::size_c<0>) {
                    return shared_sub_irq_builder<decltype(child)>{};
                } else {
                    return sub_irq_builder<decltype(child)>{};
                }
            });

        std::remove_cv_t<decltype(irqs_type)> irqs;

        template<typename IrqType, typename T>
        void constexpr add(T const & flow_description) {
            hana::for_each(irqs, [&](auto & irq) {
                irq.template add<IrqType>(flow_description);
            });
        }

        template<
            typename BuilderValue,
            typename Index>
        struct sub_value {
            constexpr static auto const & value = BuilderValue::value.irqs[Index{}];
        };

        /**
         * @return shared_irq::impl specialization optimized for size and runtime.
         */
        template<typename BuilderValue>
        [[nodiscard]] auto constexpr build() const {
            auto constexpr builder = BuilderValue::value;

            auto constexpr irq_indices =
                hana::to<hana::tuple_tag>(hana::make_range(hana::int_c<0>, hana::size(builder.irqs)));

            auto const sub_irq_impls = hana::transform(irq_indices, [&](auto i){
                constexpr auto irq = builder.irqs[i];
                return irq.template build<sub_value<BuilderValue, decltype(i)>>();
            });

            return hana::unpack(sub_irq_impls, [](auto ... sub_irq_impl_args) {
                return shared_irq_impl<ConfigT, decltype(sub_irq_impl_args)...>(sub_irq_impl_args...);
            });
        }
    };
}


#endif //CIB_INTERRUPT_BUILDER_SHARED_IRQ_BUILDER_HPP