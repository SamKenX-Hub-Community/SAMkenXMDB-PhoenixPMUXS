import Vue from 'vue'
import chai from 'chai'
import mount, { router } from '@tests/unit/setup'
import FilterDetail from '@/pages/FilterDetail'
import sinon from 'sinon'
import sinonChai from 'sinon-chai'
import { mockupAllFilters } from '@tests/unit/mockup'
chai.should()
chai.use(sinonChai)

describe('FilterDetail index', () => {
    let wrapper, axiosStub

    before(async () => {
        axiosStub = sinon.stub(Vue.prototype.$axios, 'get').returns(
            Promise.resolve({
                data: {},
            })
        )

        const filterPath = `/dashboard/filters/${mockupAllFilters[0].id}`
        if (router.history.current.path !== filterPath) await router.push(filterPath)
    })

    after(async () => {
        await axiosStub.reset()
    })

    beforeEach(async () => {
        await axiosStub.restore()
        axiosStub = sinon.stub(Vue.prototype.$axios, 'get').returns(
            Promise.resolve({
                data: {},
            })
        )
        wrapper = mount({
            shallow: false,
            component: FilterDetail,
            computed: {
                currentFilter: () => mockupAllFilters[0],
            },
        })
    })
    afterEach(async () => {
        await axiosStub.restore()
    })

    it(`Should send request to get listener, relationships service state
      and module parameters`, async () => {
        await wrapper.vm.$nextTick(async () => {
            let {
                id,
                attributes: { module: filterModule },
                relationships: {
                    services: { data: servicesData },
                },
            } = mockupAllFilters[0]

            await axiosStub.should.have.been.calledWith(`/filters/${id}`)

            let count = 1
            await servicesData.forEach(async service => {
                await axiosStub.should.have.been.calledWith(
                    `/services/${service.id}?fields[services]=state`
                )
                ++count
            })

            await axiosStub.should.have.been.calledWith(
                `/maxscale/modules/${filterModule}?fields[module]=parameters`
            )
            ++count
            axiosStub.should.have.callCount(count)
        })
    })
})
