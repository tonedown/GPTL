program simple
  use gptl
  use gptl_acc
  implicit none

  integer :: ret
  integer :: n
  integer :: sleep

!$acc parallel copyout (ret, sleep)
  ret = gptlinit_handle_gpu ('simple'//char(0), sleep)
!$acc end parallel

  write(6,*)'Sleeping 1 second on GPU...'

  ret = gptlstart ('sleep1ongpu')
!$acc parallel loop private(ret) copyin(sleep)
    do n=1,1000
      ret = gptlstart_gpu (sleep)
      ret = gptlmy_sleep (1.)
      ret = gptlstop_gpu (sleep)
    end do
!$acc end parallel
  end do
  ret = gptlstop ('sleep1ongpu')
end program simple
